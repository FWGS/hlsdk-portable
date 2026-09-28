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
#include	"game.h"
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

class CAndy : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed ( void );
	int  Classify ( void );
	void EXPORT LeapTouch ( CBaseEntity *pOther );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks ( void );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );

	void RunAI( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	void BloodSonic( int damage );

	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void DeathSound( void );
	void PainSound( void );

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

	float m_flNextFlyKickTime;
	float m_flNextBloodShockTime;
	float m_flNextGetLifeTime;
	float m_flNextBlastTime;

	float	m_dyingtime;

	int	  m_iSpriteTexture;

	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;

	Vector m_speedvec;
};

LINK_ENTITY_TO_CLASS( monster_andylow, CAndy );

void CAndy :: FollowerUse2( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
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

TYPEDESCRIPTION	CAndy::m_SaveData[] = 
{
	DEFINE_FIELD( CAndy, m_flNextFlyKickTime, FIELD_TIME ),
	DEFINE_FIELD( CAndy, m_flNextBloodShockTime, FIELD_TIME ),
	DEFINE_FIELD( CAndy, m_flNextGetLifeTime, FIELD_TIME ),
	DEFINE_FIELD( CAndy, m_flNextBlastTime, FIELD_TIME ),
	DEFINE_FIELD( CAndy, m_dyingtime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CAndy, CSquadMonster );

//=========================================================
// IRelationship - overridden because Alien Grunts are 
// Human Grunt's nemesis.
//=========================================================
int CAndy::IRelationship ( CBaseEntity *pTarget )
{
	return CSquadMonster::IRelationship( pTarget );
}


//=========================================================
// RunAI
//=========================================================
void CAndy :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->skin == 0){
		if(pev->health <= 200){
		pev->skin = 1;
		}
	}
	else if(pev->skin == 1){
		if(pev->health > 200){
		pev->skin = 0;
		}
	}

	if(pev->sequence == LookupActivity ( ACT_RUN ) || pev->sequence == LookupActivity ( ACT_RUN_SCARED )){
	m_flGroundSpeed = 360;
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

}


void CAndy :: BloodSonic ( int damage )
{
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "tyant_boss/blast.wav", 1, 0.6);

	Vector	vecSpitOffset;
	Vector  vangle;
	GetAttachment( 0, vecSpitOffset, vangle );

	vecSpitOffset.z = pev->origin.z;

	FX_Explosion(vecSpitOffset, 46 );

	// blast circles
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( vecSpitOffset.x);
		WRITE_COORD( vecSpitOffset.y);
		WRITE_COORD( vecSpitOffset.z + 16);
		WRITE_COORD( vecSpitOffset.x);
		WRITE_COORD( vecSpitOffset.y);
		WRITE_COORD( vecSpitOffset.z + 16 + 96 / .2); // reach damage radius over .3 seconds
		WRITE_SHORT( m_iSpriteTexture );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 6 ); // life
		WRITE_BYTE( 16 );  // width
		WRITE_BYTE( 0 );   // noise

		WRITE_BYTE( 255   );
		WRITE_BYTE( 32 );
		WRITE_BYTE( 32  );

		WRITE_BYTE( 255 ); //brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( vecSpitOffset.x);
		WRITE_COORD( vecSpitOffset.y);
		WRITE_COORD( vecSpitOffset.z + 16);
		WRITE_COORD( vecSpitOffset.x);
		WRITE_COORD( vecSpitOffset.y);
		WRITE_COORD( vecSpitOffset.z + 16 + ( 128 / 2 ) / .2); // reach damage radius over .3 seconds
		WRITE_SHORT( m_iSpriteTexture );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 6 ); // life
		WRITE_BYTE( 16 );  // width
		WRITE_BYTE( 0 );   // noise
		
		WRITE_BYTE( 255   );
		WRITE_BYTE( 32 );
		WRITE_BYTE( 32  );

		WRITE_BYTE( 255 ); //brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();

	::RadiusDamage_limit( pev->origin, pev, pev, damage, 320, 999, DMG_BLOOD);
}

void CAndy::Killed( entvars_t *pevAttacker, int iGib )
{
	CSquadMonster::Killed( pevAttacker, GIB_NEVER );
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
BOOL CAndy :: FCanCheckAttacks ( void )
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
BOOL CAndy :: CheckRangeAttack1 ( float flDot, float flDist )
{//����
	if (m_flNextFlyKickTime > gpGlobals->time){
	return FALSE;
	}

	if ( !FBitSet( pev->flags, FL_ONGROUND ) ){
	return FALSE;
	}

	float dist = 256;
	
	if ( m_hEnemy != NULL )
	{
		if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 128 )
		{
			return FALSE;
		}
	}

	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= dist && flDot >= 0.5){
	return TRUE;
	}

	return FALSE;
}

BOOL CAndy :: CheckMeleeAttack1 ( float flDot, float flDist )
{//�����
	float cover_dist = 100;
	float dist = 90;
	if(m_hEnemy != NULL){
		if(FClassnameIs(m_hEnemy->pev, "monster_doma_boss")){
			dist += 160;
			cover_dist += 160;
		}
	}

	if (m_flNextBloodShockTime > gpGlobals->time){
			
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
			else if (flDist >= dist + 45 )
			{
					if(pev->sequence == LookupActivity ( ACT_RUN_SCARED )){
					pev->sequence = LookupActivity ( ACT_RUN );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
			}

			return FALSE;
	}

	if ( !FBitSet( pev->flags, FL_ONGROUND ) )
	{
	return FALSE;
	}

	if ( flDist <= cover_dist)
	{
		return TRUE;
	}

	return FALSE;
}


void CAndy :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if(pev->sequence == LookupActivity ( ACT_MELEE_ATTACK1 )
	|| pev->sequence == LookupActivity ( ACT_DIESIMPLE )){
	return;
	}
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

int CAndy :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if(pev->sequence == LookupActivity ( ACT_MELEE_ATTACK1 )
	|| pev->sequence == LookupActivity ( ACT_DIESIMPLE )){
	return 0;
	}

	if(pev->deadflag == DEAD_NO){//����Ч��
		if(!(bitsDamageType & (DMG_AIR | DMG_FALL | DMG_DROWN | DMG_NERVEGAS))){
			if (RANDOM_LONG(0,100) <= 15){
			return 0;
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
void CAndy :: SetYawSpeed ( void )
{
	pev->yaw_speed = 360;
}


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CAndy :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CAndy :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	int dmg1 = 16;
//	int dmg2 = 72;
	int dmg3 = 100;

	if(pev->skin == 1){
	dmg1 = 24;
//	dmg2 = 108;
	dmg3 = 150;
	}

	switch( pEvent->event )
	{
		case 1:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 85;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg1, gpGlobals->v_forward, &tr, DMG_SLASH | DMG_NEVERGIB ); 
			ApplyMultiDamage( pev, pev );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 80, dmg1, DMG_SLASH | DMG_NEVERGIB );
				if(pHurt){
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}
			
		}
		break;

		case 2:
		{
			m_flNextFlyKickTime = gpGlobals->time + 4;
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

		//	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "mario/mario_jump.wav", 1.0, 0.6, 0, 100 );

			pev->velocity = vecJumpDir;

			if(pev->velocity.z > 300){
			pev->velocity.z = 300;
			}
		}
		break;

		case 3:
		{
			if(m_flNextGetLifeTime < gpGlobals->time){
				pev->body = 0;
				ClearSchedule();
				SetState( MONSTERSTATE_IDLE );
				m_die = 0;
				m_dieseq	= 0;
				pev->deadflag = DEAD_NO;
				pev->movetype = MOVETYPE_STEP;
				pev->health = pev->max_health;
				SetActivity( ACT_IDLE );
				Forget( bits_MEMORY_KILLED );
				m_flNextGetLifeTime = gpGlobals->time + 120;
				pev->frags = 0;
			}
			else if(pev->frags <= 3){
				pev->body = 0;
				ClearSchedule();
				SetState( MONSTERSTATE_IDLE );
				m_die = 0;
				m_dieseq	= 0;
				pev->deadflag = DEAD_NO;
				pev->movetype = MOVETYPE_STEP;
				pev->frags += 1;	
				if(pev->frags == 1){
				pev->health = pev->max_health * 0.5;
				}
				else if(pev->frags == 2){
				pev->health = pev->max_health * 0.25;
				}
				else if(pev->frags == 3){
				pev->health = pev->max_health * 0.125;
				}
				else if(pev->frags == 4){
				pev->health = pev->max_health * 0.0625;
				}
				SetActivity( ACT_IDLE );
				Forget( bits_MEMORY_KILLED );
				m_flNextGetLifeTime = gpGlobals->time + 120;
			}
		}
		break;

		case 4:
		{
			if(m_flNextBloodShockTime <= gpGlobals->time){
			BloodSonic(dmg3);
			m_flNextBloodShockTime = gpGlobals->time + 8.0;
			}
		}
		break;

		case 5:
		{
			ClearSchedule();
			SetYawSpeed();
		}
		break;

		case 6:
		{
			if(pev->frags >= 4){
			Vector org = pev->origin;
			org.z += 4;
			SpawnBlood(org, BloodColor(), 100);
			FX_Explosion(org, 236 );
			//pev->deadflag = DEAD_DEAD;
			//m_dieseq = 1;
			//UTIL_SetSize ( pev, Vector ( pev->mins.x, pev->mins.y, pev->mins.z ), Vector ( pev->maxs.x, pev->maxs.y, pev->mins.z + 4 ) );
			//StopAnimation();
			pev->frame = 255;
			m_fSequenceFinished = TRUE;
			}
		}
		break;

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

void CAndy :: LeapTouch ( CBaseEntity *pOther )
{
	if ( !pOther->pev->takedamage || pev->deadflag != DEAD_NO){
		return;
	}

	if ( pOther->Classify() == Classify() ){
		return;
	}

	int dmg;
	dmg			= 70;

	if(pev->skin == 1){
	dmg = 105;
	}

	TraceResult tr = UTIL_GetGlobalTrace( );
	TraceResult tr2;

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

		Vector vecSrc = pev->origin;
		Vector vecEnd	= vecSrc + gpGlobals->v_forward * 48;
		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

		int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
		int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),0);
		FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

		pOther->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_CLUB); 
		EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "newadd/fist_hitbod3.wav", 1.0, 0.6, 0, 100 );
		ApplyMultiDamage( pev, pev );	
	}

	SetTouch( &CAndy::DeadTouch );
}

//=========================================================
// Spawn
//=========================================================
void CAndy :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/andylow.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	pev->skin			= 0;
	pev->frags			= 0;

	pev->health			= 400;
	m_lovehate			= 80;
	m_headdef			= 2;

	m_flFieldOfView		 = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		 = MONSTERSTATE_NONE;

	m_flNextFlyKickTime = gpGlobals->time + 2;
	m_flNextBloodShockTime = gpGlobals->time + 2;
	m_flNextGetLifeTime = gpGlobals->time + 2;
	m_flNextBlastTime = gpGlobals->time + 2;

	m_afCapability		= bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;

	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_cClipSize			= 2;
	m_cAmmoLoaded		= 2;

	m_canheadcrab_mode  = 0;
	m_canbarnacle_mode  = 1;

	m_no_victdance		= 1;

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();

	m_follow_mode		= 1;

	m_candrownwater = 1;
//	m_forcefuckdoor = TRUE;
	m_chase_mode = 2;
	m_chase_failed_max = 4;

	SetUse( &CAndy::FollowerUse2 );

	m_aimenemy_mod = 6;

	pev->body = 0;

	SetTouch( &CAndy::DeadTouch );

	m_longming = 1;
	m_ignoredamage		= 1;

	m_rpgms_actor = 19;
	m_rpgms_level = 40;
	m_rpgms_exp = 0;
	m_rpgms_type = 1;

	m_new_ally_type = TRUE;
	pev->takedamage = DAMAGE_YES;
	pev->netname = MAKE_STRING( "AndyLow" );

	m_rpgms_skill1_learn = 39;//����
	m_rpgms_skill2_learn = 58;//����
	m_rpgms_skill3_learn = 59;//�����
	m_rpgms_skill4_learn = 51;//һת����
	m_rpgms_skill5_learn = 60;//ƴ��֮��
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CAndy :: Precache()
{
	PRECACHE_MODEL("models/andylow.mdl");

	PRECACHE_SOUND ("tyant_boss/blast.wav");

	m_iSpriteTexture = PRECACHE_MODEL( "sprites/shockwave.spr" );
}	

//=========================================================
// start task
//=========================================================
void CAndy :: StartTask ( Task_t *pTask )
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

	case TASK_RANGE_ATTACK1:
		{
			m_IdealActivity = ACT_RANGE_ATTACK1;
			SetTouch ( &CAndy::LeapTouch );
			break;
		}

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
void CAndy :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		{
			if ( m_fSequenceFinished )
			{
				TaskComplete();
				SetTouch( &CAndy::DeadTouch );
				m_IdealActivity = ACT_IDLE;
			}
			break;
		}
	default:
		{
			CSquadMonster :: RunTask(pTask);
		}
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CAndy :: DeathSound ( void )
{
//	EMIT_SOUND( ENT(pev), 6, "mario/mario_die.wav", 1, 0.6 );	
}

void CAndy :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
//		EMIT_SOUND( ENT(pev), 6, "mario/mario_pain.wav", 1, 0.6);

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// GruntFail
//=========================================================
Task_t	tlAndyFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)1		},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slAndyFail[] =
{
	{
		tlAndyFail,
		ARRAYSIZE ( tlAndyFail ),
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
Task_t	tlAndyCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)1		},
};

Schedule_t	slAndyCombatFail[] =
{
	{
		tlAndyCombatFail,
		ARRAYSIZE ( tlAndyCombatFail ),
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
Task_t	tlAndyVictoryDance[] =
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

Schedule_t	slAndyVictoryDance[] =
{
	{ 
		tlAndyVictoryDance,
		ARRAYSIZE ( tlAndyVictoryDance ), 
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
Task_t tlAndyEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_GRUNT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slAndyEstablishLineOfFire[] =
{
	{ 
		tlAndyEstablishLineOfFire,
		ARRAYSIZE ( tlAndyEstablishLineOfFire ),
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
Task_t	tlAndyFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slAndyFoundEnemy[] =
{
	{ 
		tlAndyFoundEnemy,
		ARRAYSIZE ( tlAndyFoundEnemy ), 
		0,
		0,
		"GruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlAndyCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_GUARD			},
	{ TASK_FACE_ENEMY,				(float)0					},
};

Schedule_t	slAndyCombatFace[] =
{
	{ 
		tlAndyCombatFace1,
		ARRAYSIZE ( tlAndyCombatFace1 ), 
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
Task_t	tlAndySignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
};

Schedule_t	slAndySignalSuppress[] =
{
	{ 
		tlAndySignalSuppress,
		ARRAYSIZE ( tlAndySignalSuppress ), 
		0,
		0,
		"SignalSuppress"
	},
};

Task_t	tlAndySuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slAndySuppress[] =
{
	{ 
		tlAndySuppress,
		ARRAYSIZE ( tlAndySuppress ), 
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
Task_t	tlAndyWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slAndyWaitInCover[] =
{
	{ 
		tlAndyWaitInCover,
		ARRAYSIZE ( tlAndyWaitInCover ), 
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
Task_t	tlAndySweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slAndySweep[] =
{
	{ 
		tlAndySweep,
		ARRAYSIZE ( tlAndySweep ), 
		
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
Task_t	tlAndyRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
};

Schedule_t	slAndyRangeAttack1[] =
{
	{ 
		tlAndyRangeAttack1,
		ARRAYSIZE ( tlAndyRangeAttack1 ), 
		0,	
		0,
		"Range Attack1"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlAndyMeleeAttack[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_MELEE_ATTACK1, 					(float)0							},
};

Schedule_t	slAndyMeleeAttack[] =
{
	{ 
		tlAndyMeleeAttack,
		ARRAYSIZE ( tlAndyMeleeAttack ), 
		0,
		0,
		"Range Attack2"
	},
};


DEFINE_CUSTOM_SCHEDULES( CAndy )
{
	slAndyFail,
	slAndyCombatFail,
	slAndyVictoryDance,
	slAndyEstablishLineOfFire,
	slAndyFoundEnemy,
	slAndyCombatFace,
	slAndySignalSuppress,
	slAndySuppress,
	slAndySweep,
	slAndyRangeAttack1,
	slAndyMeleeAttack,
};

IMPLEMENT_CUSTOM_SCHEDULES( CAndy, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void CAndy :: SetActivity ( Activity NewActivity )
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
Schedule_t *CAndy :: GetSchedule( void )
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
					if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
					{
						return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
					}
					else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
					{
						return GetScheduleOfType ( SCHED_GRUNT_SUPPRESS );
					}
					else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ))
					{
						return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
					}
					else
					{
						if( m_HenemyEnemyMe == 4){
						return GetScheduleOfType( SCHED_COMBAT_FACE );
						}
					
						return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
					}
				}
				else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
				{
					return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
				}
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
				{
					return GetScheduleOfType ( SCHED_RANGE_ATTACK1 );
				}
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ))
				{
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else if ( HasConditions( bits_COND_SEE_ENEMY ) || HasConditions( bits_COND_ENEMY_OCCLUDED ) )
				{
					if( m_HenemyEnemyMe == 4){
					return GetScheduleOfType( SCHED_COMBAT_FACE );
					}

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
Schedule_t* CAndy :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_GRUNT_ELOF_FAIL:
		{
			if(m_groundElev){
			return &slAndyCombatFace[ 0 ];
			}
			else{
			return GetScheduleOfType ( SCHED_CHASE_ENEMY_FAILED );
			}
		}
		break;
	case SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			if(m_groundElev){
			return &slAndyCombatFace[ 0 ];
			}
			else{
			return &slAndyEstablishLineOfFire[ 0 ];
			}
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			return &slAndyRangeAttack1[ 0 ];
		}
	case SCHED_MELEE_ATTACK1:
		{
			return &slAndyMeleeAttack[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slAndyCombatFace[ 0 ];
		}
	case SCHED_GRUNT_WAIT_FACE_ENEMY:
		{
			return &slAndyWaitInCover[ 0 ];
		}
	case SCHED_GRUNT_SWEEP:
		{
			return &slAndySweep[ 0 ];
		}
	case SCHED_GRUNT_FOUND_ENEMY:
		{
			return &slAndyFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			return &slAndyVictoryDance[ 0 ];
		}
	case SCHED_GRUNT_SUPPRESS:
		{
			return &slAndySuppress[ 0 ];
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slAndyCombatFail[ 0 ];
			}

			return &slAndyFail[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}