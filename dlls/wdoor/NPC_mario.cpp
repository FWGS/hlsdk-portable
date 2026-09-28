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


//=========================================================
// Bullsquid's spit projectile
//=========================================================
class CMarioFireBall : public CBaseEntity
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

LINK_ENTITY_TO_CLASS( fire_ball, CMarioFireBall );

TYPEDESCRIPTION	CMarioFireBall::m_SaveData[] = 
{
	DEFINE_FIELD( CMarioFireBall, m_maxFrame, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CMarioFireBall, CBaseEntity );

void CMarioFireBall:: Spawn( void )
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->classname = MAKE_STRING( "fire_ball" );
	
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "sprites/mfireball.spr");
	pev->frame = 0;
	pev->rendermode = kRenderTransAdd;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 255;
	pev->rendercolor.z = 255;
	pev->renderamt = 255;
	pev->scale = 0.5;

	pev->health = 40;

	UTIL_SetSize( pev, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_BEAMFOLLOW );
	WRITE_SHORT( entindex() );		// entity, attachment
	WRITE_SHORT(g_sModelIndexTrail );	// model
	WRITE_BYTE( 4 ); // life
	WRITE_BYTE( 8 );  // width
	WRITE_BYTE( 255 );	// R
	WRITE_BYTE( 192 );	// G
	WRITE_BYTE( 128 );	// B
	WRITE_BYTE( 192 );	// brightness
	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
	pev->gravity = 4.0;
}

void CMarioFireBall::Animate( void )
{
	pev->nextthink = gpGlobals->time + 0.1;
	pev->health--;

	if(pev->health <= 0){
	FX_Explosion( pev->origin, 136);
	SetThink ( &CMarioFireBall::SUB_Remove );
	pev->nextthink = gpGlobals->time;
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

void CMarioFireBall::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CMarioFireBall *pSpit = GetClassPtr( (CMarioFireBall *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->pev->angles = UTIL_VecToAngles (pSpit->pev->velocity);
	pSpit->SetThink ( &CMarioFireBall::Animate );
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CMarioFireBall :: Touch ( CBaseEntity *pOther )
{
	if ( pOther->pev->takedamage ){
		entvars_t *pevOwner = VARS( pev->owner );

		if(pOther->pev->takedamage){
		pOther->TakeDamage ( pev, pevOwner, 60, DMG_BURN );
		}

		::RadiusDamage_limit( pev->origin, pev, pevOwner, 40, 160, CLASS_PLAYER_ALLY, DMG_BURN );

		FX_Explosion( pev->origin, 136);

		SetThink ( &CMarioFireBall::SUB_Remove );
		pev->nextthink = gpGlobals->time;
	}
}

class CMario : public CSquadMonster
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

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	
	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );
//	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	virtual int	ObjectCaps( void ) { return CBaseMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	void EXPORT		FollowerUse2( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int IRelationship ( CBaseEntity *pTarget );

	static const char *pMarioSounds[];

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextPainTime;

	float m_flNextFireBallTime;
	float m_flNextJumpTime;
	float m_flNextRotTime;
	float m_flNext1UpTime;
	float m_flKillBeamTime;
//	float m_flNanomachineTime;
	float m_flNextGodWindFireTime;

	float	m_dyingtime;

	int	  m_iSpriteTexture;

	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;

	Vector m_speedvec;
};

LINK_ENTITY_TO_CLASS( monster_mario, CMario );

const char *CMario::pMarioSounds[] = 
{
	"mario/mario_attack1.wav",
	"mario/mario_attack2.wav",
	"mario/mario_attack3.wav",
	"mario/mario_attack4.wav",
};

void CMario :: FollowerUse2( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
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

TYPEDESCRIPTION	CMario::m_SaveData[] = 
{
	DEFINE_FIELD( CMario, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CMario, m_flNextFireBallTime, FIELD_TIME ),
	DEFINE_FIELD( CMario, m_flNextJumpTime, FIELD_TIME ),
	DEFINE_FIELD( CMario, m_flNextRotTime, FIELD_TIME ),
	DEFINE_FIELD( CMario, m_flNext1UpTime, FIELD_TIME ),
	DEFINE_FIELD( CMario, m_flKillBeamTime, FIELD_TIME ),
//	DEFINE_FIELD( CMario, m_flNanomachineTime, FIELD_TIME ),
	DEFINE_FIELD( CMario, m_flNextGodWindFireTime, FIELD_TIME ),
	DEFINE_FIELD( CMario, m_dyingtime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CMario, CSquadMonster );

//=========================================================
// IRelationship - overridden because Alien Grunts are 
// Human Grunt's nemesis.
//=========================================================
int CMario::IRelationship ( CBaseEntity *pTarget )
{
	if ( FClassnameIs( pTarget->pev, "monster_luigi" ) )
	{
		return R_NM;
	}

	return CSquadMonster::IRelationship( pTarget );
}


//=========================================================
// RunAI
//=========================================================
void CMario :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->sequence == LookupActivity ( ACT_RUN )){
	m_flGroundSpeed = 360;
	}

	if(pev->movetype == MOVETYPE_FLY){//����bug
		if(pev->sequence == LookupActivity ( ACT_IDLE ) 
		|| pev->sequence == LookupActivity ( ACT_RUN )){
			pev->movetype = MOVETYPE_STEP;
		}
	}

	if(pev->sequence == LookupActivity ( ACT_GUARD )){
	m_duckseq = 4;
	}
	else if(m_duckseq != 0){
	m_duckseq = 0;
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
		m_freeze_def = 0;
		}
	}

	if(pev->body == 1){
		pev->impulse--;
		if(pev->impulse <= 0){
		pev->body = 0;
		}
	}

	if(pev->armortype > 0){
		pev->armortype--;
		if(pev->armortype <= 0){
			pev->renderfx = 0;
			pev->takedamage = DAMAGE_YES;
			m_groundElev2 = FALSE;//Bug Fix 2.0 ����·������£��رռ��
		}
	}

	if(m_hEnemy != NULL && pev->armorvalue == 0 && pev->deadflag == DEAD_NO
	&& pev->sequence != LookupActivity ( ACT_MELEE_ATTACK2 ) ){
			if(( pev->origin.z - m_hEnemy->pev->origin.z ) > 64){

				if(pev->sequence == LookupActivity ( ACT_MELEE_ATTACK1 )){
					if(( pev->origin - m_hEnemy->pev->origin).Length2D() > 32){
					m_speedvec = (m_hEnemy->pev->origin - pev->origin).Normalize() * 32;
					pev->velocity.x = m_speedvec.x;
					pev->velocity.y = m_speedvec.y;
					}
				}

				TraceResult tr;
				UTIL_MakeVectors(pev->angles);
				Vector vecSrc	= pev->origin;
				Vector vecEnd	= m_hEnemy->Center();
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

				if ( tr.flFraction < 1.0 && pEntity->pev->takedamage 
				&& ( vecSrc - tr.vecEndPos).Length() <= 72 ){
					int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
					int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
					FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

					ClearMultiDamage( );
					pEntity->TraceAttack(pev, 100, gpGlobals->v_forward, &tr, DMG_CLUB | DMG_NEVERGIB ); 
					ApplyMultiDamage( pev, pev );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "mario/mario_hit.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					FX_Explosion( tr.vecEndPos, 49);

					pev->flags &= ~FL_ONGROUND;
					pev->velocity.z += 300;
					pev->armorvalue = 15;
				}	
			}
	}
	else if(pev->armorvalue > 0){
	pev->armorvalue--;
	}

	if(m_flKillBeamTime > 0 && m_flKillBeamTime < gpGlobals->time){
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 4 );		// entity, attachment
	MESSAGE_END();

	pev->movetype = MOVETYPE_STEP;

	m_flKillBeamTime = 0;
	}

}


void CMario::Killed( entvars_t *pevAttacker, int iGib )
{
	SetTouch( &CMario::DeadTouch );

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
BOOL CMario :: FCanCheckAttacks ( void )
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
BOOL CMario :: CheckRangeAttack1 ( float flDot, float flDist )
{//����
	if (m_flNextFireBallTime > gpGlobals->time){
	return FALSE;
	}

	float dist = 1200;
	
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

BOOL CMario :: CheckRangeAttack2 ( float flDot, float flDist )
{//��Ծ
	if (m_flNextJumpTime > gpGlobals->time || m_playerguardian_mode == 1){
	return FALSE;
	}

	if ( !FBitSet( pev->flags, FL_ONGROUND ) )
	{
	return FALSE;
	}

	float dist = 600;
	
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= dist && flDot >= 0.5){
	return TRUE;
	}

	return FALSE;
}

BOOL CMario :: CheckMeleeAttack2 ( float flDot, float flDist )
{//�޵з����
	if (m_flNextGodWindFireTime > gpGlobals->time || m_playerguardian_mode == 1){
	return FALSE;
	}

	if ( !FBitSet( pev->flags, FL_ONGROUND ) )
	{
	return FALSE;
	}

	float dist = 900;
	
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= dist && flDot >= 0.5){
	return TRUE;
	}

	return FALSE;
}

BOOL CMario :: CheckMeleeAttack1 ( float flDot, float flDist )
{//��ת����
	if (m_flNextRotTime > gpGlobals->time){
	return FALSE;
	}

	if ( !FBitSet( pev->flags, FL_ONGROUND ) )
	{
	return FALSE;
	}

	float cover_dist = 100;

	if(m_playerguardian_mode == 1){
	cover_dist = 150;
	}

	if ( m_hEnemy != NULL )
	{
		if(FClassnameIs(m_hEnemy->pev, "monster_doma_boss")){
		cover_dist += 160;
		}
	}

	if ( flDist <= cover_dist)
	{
		return TRUE;
	}

	return FALSE;
}


//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CMario :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if(pev->sequence == LookupActivity ( ACT_MELEE_ATTACK2 )){
	return 0;
	}

	if(pev->sequence == LookupActivity ( ACT_GUARD )){
	flDamage *= 0.5;
	}

	/*
	if(m_flNanomachineTime < gpGlobals->time && pev->body == 0
	&& pev->deadflag == DEAD_NO){
	pev->body = 1;
	pev->impulse = 150;
	m_flNanomachineTime = gpGlobals->time + 45;
	}

	if(pev->body == 1){
	flDamage *= 0.1;
	}
	*/

	m_alert	= 100;

	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CMario :: SetYawSpeed ( void )
{
	pev->yaw_speed = 360;
}


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CMario :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMario :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case 5:
		{
			if(m_flNextFireBallTime <= gpGlobals->time){
			Vector	vecSpitOffset;
			Vector	vecSpitDir;
			Vector  vangle;

			UTIL_MakeVectors ( pev->angles );

			// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
			// we should be able to read the position of bones at runtime for this info.
			GetAttachment( 0, vecSpitOffset, vangle );

			vecSpitDir = ( m_hEnemy->Center() - vecSpitOffset ).Normalize();

			Vector Sog = vecSpitOffset + gpGlobals->v_forward * 4;

			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "mario/mario_fireball.wav", 1.0, 0.6,0,100);

			CMarioFireBall::Shoot( pev, vecSpitOffset, vecSpitDir * 1500 );
				m_cAmmoLoaded--;
				if(m_cAmmoLoaded <= 0){
				m_flNextFireBallTime = gpGlobals->time + 8;
				m_cAmmoLoaded = 2;
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
				Vector vecShootOrigin = pev->origin + Vector(0,0,64);
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

		case 6:
		{
			pev->armorvalue = 0;
			m_flNextJumpTime = gpGlobals->time + 6;
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

				if (height < 20){
					height = 20;
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
				
				if (distance > 750)
				{
					vecJumpDir = vecJumpDir * ( 750.0 / distance );
				}
			}
			else
			{
				// jump hop, don't care where
				vecJumpDir = Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z ) * 350;
			}

			EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "mario/mario_jump.wav", 1.0, 0.6, 0, 100 );

			pev->velocity = vecJumpDir;

			if(( pev->origin - m_hEnemy->pev->origin).Length() >= 512){
			pev->velocity.z *= 1.8;
			}
			else{
			pev->velocity.z *= 1.2;
			}

			if(pev->velocity.z > 600){
			pev->velocity.z = 600;
			}
		}
		break;

		case 7:
		{
		//	m_flNextJumpTime = gpGlobals->time + 8;
		}
		break;

		case 1:
		{
			if(m_flNextRotTime <= gpGlobals->time){
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail);	// model
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 4 );  // width
			WRITE_BYTE( 128 );	// R
			WRITE_BYTE( 192 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail);	// model
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 4 );  // width
			WRITE_BYTE( 128 );	// R
			WRITE_BYTE( 192 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			::RadiusDamage_limit( Center(), pev, pev, 60, 150, CLASS_PLAYER_ALLY, DMG_SLASH);

			EMIT_SOUND_DYN( edict(), 6, RANDOM_SOUND_ARRAY(pMarioSounds), 1.0, 0.6, 0, 100);

			UTIL_MakeVectors(pev->angles);
			pev->velocity = gpGlobals->v_forward * 150;

			pev->flags &= ~FL_ONGROUND;
			pev->velocity.z += 600;

			pev->armorvalue = 0;
			m_flKillBeamTime = gpGlobals->time + 0.6;
			m_flNextRotTime = gpGlobals->time + 4;
			}
		}
		break;

		case 2:
		{
			::RadiusDamage_limit( Center(), pev, pev, 60, 150, CLASS_PLAYER_ALLY, DMG_SLASH);
		}
		break;

		case 15:
		{
			if(m_flNextGodWindFireTime <= gpGlobals->time){
			pev->flags &= ~FL_ONGROUND;
			pev->velocity.z += 650;

			m_dyingtime = -60;
			m_canbarnacle_mode  = 0;
			m_freeze_def = 2;//���Ό��

			EMIT_SOUND_DYN( edict(), 6, RANDOM_SOUND_ARRAY(pMarioSounds), 1.0, 0.6, 0, 100);
			}
		}
		break;

		case 16:
		{
			if(m_flNextGodWindFireTime <= gpGlobals->time){
			pev->velocity = g_vecZero;
			pev->movetype = MOVETYPE_FLY;
			}
		}
		break;

		case 17:
		{
			if(m_flNextGodWindFireTime <= gpGlobals->time && m_hEnemy != NULL){
			m_flNextGodWindFireTime = gpGlobals->time + 30.0;
		
			EMIT_SOUND_DYN( ENT(pev), 6, "rmxp/134-Wind03.wav", 1.0, 0.5,0,100);

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail);	// model
			WRITE_BYTE( 6 ); // life
			WRITE_BYTE( 6 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 32 );	// G
			WRITE_BYTE( 32 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 4 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail);	// model
			WRITE_BYTE( 6 ); // life
			WRITE_BYTE( 6 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 32 );	// G
			WRITE_BYTE( 32 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			pev->armorvalue = 0;
			m_flKillBeamTime = gpGlobals->time + 4.5;
			}
			else{
			ClearSchedule();
			SetYawSpeed();
			}
		}
		break;

		case 18:
		{
			pev->movetype = MOVETYPE_BOUNCE;
			if(m_hEnemy != NULL){
			SetTouch ( &CMario::LeapTouch );
			Vector	vecSpitDir;
			UTIL_MakeVectors ( pev->angles );
			vecSpitDir = ( m_hEnemy->Center() - Center() ).Normalize();
			pev->velocity = vecSpitDir * 1500;
			}
			else{
			pev->velocity.x += RANDOM_LONG(-300,300);
			pev->velocity.y += RANDOM_LONG(-300,300);
			pev->velocity.z += 350;
			}
		}
		break;

		case 19:
		{
			pev->movetype = MOVETYPE_STEP;
		}
		break;

		case 13:
		{
			ClearSchedule();
			SetYawSpeed();
		}
		break;

		case 14:
		{
			if ( FBitSet( pev->flags, FL_ONGROUND ) && pev->velocity.Length() <= 80)
			{
				ClearSchedule();
				SetTouch( &CMario::DeadTouch );
			}
		}
		break;

		case 10:
		{
			if(pev->weapons <= 0){//��͸��
			pev->frame = 255;
			pev->skin = 1;
			m_fSequenceFinished = TRUE;
			pev->weapons = 3;//������������
			}
		}
		break;

		case 11:
		{
		//	pev->body = 1;
		}
		break;

		case 12:
		{
			if(pev->skin == 0 && pev->weapons > 0){
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
			EMIT_SOUND( ENT(pev), 6, "mario/mario_1up.wav", 1, 0.4 );
			FX_Explosion( Center(), 42);
			pev->renderfx = kRenderFxGlowShell;

			pev->rendercolor.x = 255;
			pev->rendercolor.y = 255;
			pev->renderamt = 2;
			pev->takedamage = DAMAGE_NO;
			pev->armortype = 60;
			m_groundElev2 = TRUE;//Bug Fix 2.0 ����·������£��������

			pev->weapons--;
			}
			//m_flNext1UpTime = gpGlobals->time + 120;
		}
		break;

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

void CMario :: LeapTouch ( CBaseEntity *pOther )
{
	if ( !pOther->pev->takedamage || pev->deadflag != DEAD_NO || pev->armorvalue != 0){
		return;
	}

	if ( pOther->Classify() == CLASS_PLAYER_ALLY || pOther->Classify() == CLASS_PLAYER){
		return;
	}

	TraceResult tr;

	// Don't hit if back on ground
	if ( !FBitSet( pev->flags, FL_ONGROUND ) )
	{
		pev->velocity.x = -pev->velocity.x;
		pev->velocity.y = -pev->velocity.y;
		pev->velocity.z += 300;

		ClearMultiDamage( );

		Vector vecSrc = Center();
		Vector vecEnd = pOther->Center();
		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

		int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
		int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),0);
		FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

		if(pev->sequence == LookupActivity ( ACT_MELEE_ATTACK2 )){
		pOther->TraceAttack(pev, 400, gpGlobals->v_forward, &tr, DMG_SLASH); 
		EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "mario/mario_skill_hit.wav", 1.0, 0.6, 0, 100 );
		FX_Explosion( tr.vecEndPos, 47);
		SetTouch( &CMario::DeadTouch );
		}
		else{
		pOther->TraceAttack(pev, 100, gpGlobals->v_forward, &tr, DMG_CLUB | DMG_NEVERGIB); 
		EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "mario/mario_hit.wav", 1.0, 0.6, 0, 100 );
		FX_Explosion( tr.vecEndPos, 49);
		}
		ApplyMultiDamage( pev, pev );	
	}

	if(pev->sequence != LookupActivity ( ACT_MELEE_ATTACK2 )){
	ClearSchedule();
	SetTouch( &CMario::DeadTouch );
	}
}

//=========================================================
// Spawn
//=========================================================
void CMario :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/mario.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	pev->body			= 0;

	pev->health			= 800;
	m_lovehate			= 80;

	m_flFieldOfView		 = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		 = MONSTERSTATE_NONE;

	m_flNextFireBallTime = gpGlobals->time + 2;
	m_flNextJumpTime = gpGlobals->time + 2;
	m_flNextRotTime = gpGlobals->time + 2;
	m_flNext1UpTime = gpGlobals->time + 2;

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
	m_headdef			= 2;

	m_ignoredamage		= 2;

	m_candrownwater = 1;
//	m_forcefuckdoor = TRUE;
	m_chase_mode = 2;
	m_chase_failed_max = 4;

	SetUse( &CMario::FollowerUse2 );

	m_aimenemy_mod = 6;

	pev->body = 0;

	SetTouch( &CMario::DeadTouch );

	m_longming = 1;

	m_rpgms_actor = 18;
	m_rpgms_level = 80;
	m_rpgms_exp = 0;
	m_rpgms_type = 1;

	m_new_ally_type = TRUE;
	pev->netname = MAKE_STRING( "Mario" );

	m_rpgms_skill1_learn = 53;
	m_rpgms_skill2_learn = 54;
	m_rpgms_skill3_learn = 55;
	m_rpgms_skill4_learn = 56;
	m_rpgms_skill5_learn = 57;
	m_rpgms_skill6_learn = 79;//�޵з����

	pev->armorvalue = 0;
	pev->takedamage = DAMAGE_YES;

	pev->weapons = 3;//3����
	pev->skin = 0;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMario :: Precache()
{
	PRECACHE_MODEL("models/mario.mdl");
	PRECACHE_MODEL("sprites/mfireball.spr");

	PRECACHE_SOUND_ARRAY(pMarioSounds);

	PRECACHE_SOUND("mario/mario_die.wav");
	PRECACHE_SOUND("mario/mario_pain.wav");
	PRECACHE_SOUND("mario/mario_fireball.wav");
	PRECACHE_SOUND("mario/mario_hit.wav");
	PRECACHE_SOUND("mario/mario_jump.wav");
	PRECACHE_SOUND("mario/mario_die.wav");
	PRECACHE_SOUND("mario/mario_1up.wav");
	PRECACHE_SOUND("mario/mario_skill_hit.wav");

	PRECACHE_SOUND("majo/flame_hitwall.wav");

	PRECACHE_SOUND("rmxp/134-Wind03.wav");

	m_iSpriteTexture = PRECACHE_MODEL( "sprites/shockwave.spr" );
}	

//=========================================================
// start task
//=========================================================
void CMario :: StartTask ( Task_t *pTask )
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

	case TASK_RANGE_ATTACK2:
		{
			m_IdealActivity = ACT_RANGE_ATTACK2;
			SetTouch ( &CMario::LeapTouch );
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
void CMario :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK2:
		{
			if ( m_fSequenceFinished )
			{
				TaskComplete();
				SetTouch( &CMario::DeadTouch );
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
void CMario :: DeathSound ( void )
{
	EMIT_SOUND( ENT(pev), 6, "mario/mario_die.wav", 1, 0.6 );	
}

void CMario :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
		EMIT_SOUND( ENT(pev), 6, "mario/mario_pain.wav", 1, 0.6);

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// GruntFail
//=========================================================
Task_t	tlMarioFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)1		},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slMarioFail[] =
{
	{
		tlMarioFail,
		ARRAYSIZE ( tlMarioFail ),
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
Task_t	tlMarioCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)1		},
};

Schedule_t	slMarioCombatFail[] =
{
	{
		tlMarioCombatFail,
		ARRAYSIZE ( tlMarioCombatFail ),
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
Task_t	tlMarioVictoryDance[] =
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

Schedule_t	slMarioVictoryDance[] =
{
	{ 
		tlMarioVictoryDance,
		ARRAYSIZE ( tlMarioVictoryDance ), 
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
Task_t tlMarioEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_GRUNT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slMarioEstablishLineOfFire[] =
{
	{ 
		tlMarioEstablishLineOfFire,
		ARRAYSIZE ( tlMarioEstablishLineOfFire ),
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
Task_t	tlMarioFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slMarioFoundEnemy[] =
{
	{ 
		tlMarioFoundEnemy,
		ARRAYSIZE ( tlMarioFoundEnemy ), 
		0,
		0,
		"GruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlMarioCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_GUARD			},
	{ TASK_FACE_ENEMY,				(float)0					},
};

Schedule_t	slMarioCombatFace[] =
{
	{ 
		tlMarioCombatFace1,
		ARRAYSIZE ( tlMarioCombatFace1 ), 
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
Task_t	tlMarioSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
};

Schedule_t	slMarioSignalSuppress[] =
{
	{ 
		tlMarioSignalSuppress,
		ARRAYSIZE ( tlMarioSignalSuppress ), 
		0,
		0,
		"SignalSuppress"
	},
};

Task_t	tlMarioSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slMarioSuppress[] =
{
	{ 
		tlMarioSuppress,
		ARRAYSIZE ( tlMarioSuppress ), 
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
Task_t	tlMarioWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slMarioWaitInCover[] =
{
	{ 
		tlMarioWaitInCover,
		ARRAYSIZE ( tlMarioWaitInCover ), 
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
Task_t	tlMarioSweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slMarioSweep[] =
{
	{ 
		tlMarioSweep,
		ARRAYSIZE ( tlMarioSweep ), 
		
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
Task_t	tlMarioRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
};

Schedule_t	slMarioRangeAttack1[] =
{
	{ 
		tlMarioRangeAttack1,
		ARRAYSIZE ( tlMarioRangeAttack1 ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED,	
		0,
		"Range Attack1"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlMarioRangeAttack2[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
};

Schedule_t	slMarioRangeAttack2[] =
{
	{ 
		tlMarioRangeAttack2,
		ARRAYSIZE ( tlMarioRangeAttack2 ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Range Attack2"
	},
};


DEFINE_CUSTOM_SCHEDULES( CMario )
{
	slMarioFail,
	slMarioCombatFail,
	slMarioVictoryDance,
	slMarioEstablishLineOfFire,
	slMarioFoundEnemy,
	slMarioCombatFace,
	slMarioSignalSuppress,
	slMarioSuppress,
	slMarioSweep,
	slMarioRangeAttack1,
	slMarioRangeAttack2,
};

IMPLEMENT_CUSTOM_SCHEDULES( CMario, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void CMario :: SetActivity ( Activity NewActivity )
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
Schedule_t *CMario :: GetSchedule( void )
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
					else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK2 ) )
					{
						return GetScheduleOfType ( SCHED_MELEE_ATTACK2 );
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
				else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK2 ) )
				{
					return GetScheduleOfType ( SCHED_MELEE_ATTACK2 );
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
Schedule_t* CMario :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_GRUNT_ELOF_FAIL:
		{
			if(m_groundElev){
			return &slMarioCombatFace[ 0 ];
			}
			else{
			return GetScheduleOfType ( SCHED_CHASE_ENEMY_FAILED );
			}
		}
		break;
	case SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			if(m_groundElev){
			return &slMarioCombatFace[ 0 ];
			}
			else{
			return &slMarioEstablishLineOfFire[ 0 ];
			}
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			return &slMarioRangeAttack1[ 0 ];
		}
	case SCHED_RANGE_ATTACK2:
		{
			return &slMarioRangeAttack2[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slMarioCombatFace[ 0 ];
		}
	case SCHED_GRUNT_WAIT_FACE_ENEMY:
		{
			return &slMarioWaitInCover[ 0 ];
		}
	case SCHED_GRUNT_SWEEP:
		{
			return &slMarioSweep[ 0 ];
		}
	case SCHED_GRUNT_FOUND_ENEMY:
		{
			return &slMarioFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			return &slMarioVictoryDance[ 0 ];
		}
	case SCHED_GRUNT_SUPPRESS:
		{
			return &slMarioSuppress[ 0 ];
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slMarioCombatFail[ 0 ];
			}

			return &slMarioFail[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}