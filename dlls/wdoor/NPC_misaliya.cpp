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


class CIceTip : public CBaseEntity
{
public:
	void Spawn( void );

	void EXPORT Thinking( void );
};

LINK_ENTITY_TO_CLASS( ice_tip, CIceTip );


void CIceTip:: Spawn( void )
{
	pev->movetype = MOVETYPE_NONE;
	pev->classname = MAKE_STRING( "ice_tip" );
	
	pev->solid = SOLID_NOT;
	
	SET_MODEL(ENT(pev), "models/ice_tip.mdl");
	pev->body = 0;

	UTIL_SetSize( pev, g_vecZero, g_vecZero );

	SetThink ( &CIceTip::Thinking );
	pev->nextthink = gpGlobals->time + 1.5;

	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;
	pev->frame = 0;
	pev->sequence = 0;

	SetBits(pev->effects, EF_DIMLIGHT);

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "war3/impalehit.wav", VOL_NORM, ATTN_NORM); 
}


void CIceTip::Thinking( void )
{
	entvars_t *pevOwner = VARS( pev->owner );
	::RadiusDamage_limit( pev->origin, pev, pevOwner, 90, 180, 623, DMG_FREEZE);
	//Bug Fix 3.0 ��������ж������ٲ���Ҫ����Ѫ�Ѿ��˺�Ч��
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "misaliya/frostnova.wav", VOL_NORM, ATTN_NORM); 
	FX_Trail( Center(), entindex(), PROJ_ICE_DETONATE );
	SetThink ( NULL );
	UTIL_Remove( this );
	return;

}

class CIceBreak : public CBaseEntity
{
public:
	void Spawn( void );

	void EXPORT Thinking( void );
};

LINK_ENTITY_TO_CLASS( ice_break, CIceBreak );

void CIceBreak:: Spawn( void )
{
	pev->movetype = MOVETYPE_NONE;
	pev->classname = MAKE_STRING( "ice_break" );
	
	pev->solid = SOLID_NOT;
	
	SET_MODEL(ENT(pev), "models/ice_tip.mdl");
	pev->body = 1;

	UTIL_SetSize( pev, g_vecZero, g_vecZero );

	SetThink ( &CIceBreak::Thinking );
	pev->nextthink = gpGlobals->time + 6.0;

	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;
	pev->frame = 0;
	pev->sequence = 0;

	SetBits(pev->effects, EF_DIMLIGHT);

	FX_Trail( Center(), entindex(), PROJ_CLUSTERBOMB_DETONATE );

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "misaliya/frostnova2.wav", VOL_NORM, ATTN_NORM); 
}

void CIceBreak::Thinking( void )
{
//	entvars_t *pevOwner = VARS( pev->owner );
//	::RadiusDamage_limit( pev->origin, pev, pevOwner, 90, 180, CLASS_PLAYER_ALLY, DMG_FREEZE);
//	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "misaliya/frostnova.wav", VOL_NORM, ATTN_NORM); 
//	FX_Trail( Center(), entindex(), PROJ_ICE_DETONATE );
	SetThink ( NULL );
	UTIL_Remove( this );
	return;

}

class CProctShield : public CBaseEntity
{
public:
	void Spawn( void );

	void EXPORT AnimateThink( void );
};

LINK_ENTITY_TO_CLASS( magic_protec_shield, CProctShield );
void CProctShield:: Spawn( void )
{
	pev->solid = SOLID_NOT;
	
	SET_MODEL(ENT(pev), "models/magic_protec2.mdl");
	pev->frame = 0;
	pev->framerate = 1.0;

	UTIL_SetSize( pev, g_vecZero, g_vecZero);

	pev->health = 300;

	pev->classname = MAKE_STRING( "magic_protec_shield" );

	SetThink ( &CProctShield::AnimateThink );
	pev->nextthink = gpGlobals->time + 0.05;
}

void CProctShield::AnimateThink( void )
{
	pev->nextthink = gpGlobals->time + 0.05;
	pev->health--;

	if(pev->health <= 0){
		UTIL_Remove( this );
		return;
	}

	if(!FNullEnt(pev->owner)){
		entvars_t	*pevOwner;
		pevOwner = VARS( pev->owner );
		if(pevOwner->deadflag != DEAD_NO){
		pev->health = 0;
		}
	}
}

//=========================================================
// Bullsquid's spit projectile
//=========================================================
class CShockBall : public CBaseEntity
{
public:
	void Spawn( void );

	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity,int type );
	void Touch( CBaseEntity *pOther );
	void EXPORT Animate( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int  m_maxFrame;
};

LINK_ENTITY_TO_CLASS( shock_ball, CShockBall );

TYPEDESCRIPTION	CShockBall::m_SaveData[] = 
{
	DEFINE_FIELD( CShockBall, m_maxFrame, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CShockBall, CBaseEntity );

void CShockBall:: Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "shock_ball" );
	
	pev->solid = SOLID_BBOX;
	
	SET_MODEL(ENT(pev), "sprites/anim_spr2.spr");
	pev->frame = 0;
	pev->scale = 0.2;

	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 200;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 255;
	pev->rendercolor.z = 255;

	UTIL_SetSize( pev, Vector( -1, -1, 1), Vector(1, 1, 1) );

	m_maxFrame = 0;
}

void CShockBall::Animate( void )
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

		if(m_maxFrame == 0){
		m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
		}

		if ( pev->frame++ )
		{
			if ( pev->frame > m_maxFrame )
			{
				pev->frame = 0;
			}
		}
}

void CShockBall::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity,int type )
{
	CShockBall *pSpit = GetClassPtr( (CShockBall *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->pev->angles = UTIL_VecToAngles (pSpit->pev->velocity);
	pSpit->SetThink ( &CShockBall::Animate );
	pSpit->pev->nextthink = gpGlobals->time + 0.1;

	if(type == 2){
	pSpit->pev->dmg = 200;
	FX_Trail(pSpit->pev->origin, pSpit->entindex(), PROJ_SHOCK2);
	}
	else{
	pSpit->pev->dmg = 100;
	FX_Trail(pSpit->pev->origin, pSpit->entindex(), PROJ_SHOCK);
	}
}

void CShockBall :: Touch ( CBaseEntity *pOther )
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

		

	TraceResult tr = UTIL_GetGlobalTrace( );
	if (tr.pHit == pOther->edict())//ֱ������!
	{
		ClearMultiDamage( );
		pOther->TraceAttack(pev, pev->dmg, gpGlobals->v_forward, &tr, DMG_SHOCK ); 

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

	if(pev->dmg >= 200){
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/shock_blast.wav", 1, ATTN_NORM, 0, iPitch + 20 );
	FX_Trail( pev->origin, entindex(), PROJ_SHOCK_DETONATE);
	}
	else{
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/shock_blast.wav", 1, ATTN_NORM, 0, iPitch );
	FX_Trail( pev->origin, entindex(), PROJ_SHOCKPOWER_DETONATE);
	}

	SetThink ( &CShockBall::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}



//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_GRUNT_NOFIRE	( bits_COND_SPECIAL1 )

class CMisaliya : public CSquadMonster
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
	void PainSound( void );

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
	float m_flNextPainTime;
	float m_flNextShootTime;
	float m_flNextShoot2Time;
	float m_flNextIcetip;
	float m_flNextHealTime;
	float m_flNextDefine;
	float m_flLastEnemySightTime;
	float m_flNextGrenadeCheck;
	float m_flNextIceBreak;
	float m_flNextBlastSkillTime;

	Vector  m_vecTossVelocity;

	BOOL	m_fThrowGrenade;

	BOOL	m_fHealPlayer;
	BOOL	m_fHealMyself;

	EHANDLE	mps_ent;

	float	m_dyingtime;
	int		m_dyinguse;

	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;
};


LINK_ENTITY_TO_CLASS( monster_misaliya, CMisaliya );

void CMisaliya :: FollowerUse2( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
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

TYPEDESCRIPTION	CMisaliya::m_SaveData[] = 
{
	DEFINE_FIELD( CMisaliya, m_fHealPlayer, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMisaliya, m_fHealMyself, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMisaliya, m_flNextHealTime, FIELD_TIME ),
	DEFINE_FIELD( CMisaliya, m_flNextShootTime, FIELD_TIME ),
	DEFINE_FIELD( CMisaliya, m_flNextShoot2Time, FIELD_TIME ),
	DEFINE_FIELD( CMisaliya, m_flNextBlastSkillTime, FIELD_TIME ),
	DEFINE_FIELD( CMisaliya, m_flNextIcetip, FIELD_TIME ),
	DEFINE_FIELD( CMisaliya, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CMisaliya, m_flNextDefine, FIELD_TIME ),
	DEFINE_FIELD( CMisaliya, m_flNextIceBreak, FIELD_TIME ),
	DEFINE_FIELD( CMisaliya, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMisaliya, m_dyingtime, FIELD_TIME ),
	DEFINE_FIELD( CMisaliya, m_dyinguse, FIELD_INTEGER ),
	DEFINE_FIELD( CMisaliya, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CMisaliya, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CMisaliya, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CMisaliya, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMisaliya, mps_ent, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CMisaliya, CSquadMonster );

//=========================================================
// IRelationship - overridden because Alien Grunts are 
// Human Grunt's nemesis.
//=========================================================
int CMisaliya::IRelationship ( CBaseEntity *pTarget )
{
	return CSquadMonster::IRelationship( pTarget );
}


//=========================================================
// RunAI
//=========================================================
void CMisaliya :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(m_rpgms_skill8_learn == 43 && pev->max_health < 360){
	pev->max_health = 360;
	pev->health = pev->max_health;
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

	if(m_hEnemy != NULL){
		if ( pev->health <= pev->max_health * 0.6 )
		{
			if(m_flNextHealTime <= gpGlobals->time && !m_fHealPlayer && !m_fHealMyself
			&& pev->sequence != LookupActivity ( ACT_RANGE_ATTACK2 )
			&& pev->sequence != LookupActivity ( ACT_HOP )
			&& pev->sequence != LookupActivity ( ACT_LEAP )){//�����Լ�
			m_fHealMyself = TRUE;
			m_igonre_npc = 10;
			}
		}
		else if(m_enemyfollower == 1 && m_lovehate > 0){

			if(m_hEnemy->IsPlayer() && pev->sequence == LookupActivity ( ACT_RANGE_ATTACK1 )){
			SetActivity( ACT_IDLE );//�Ѿ��˺�ֹͣ����
			ClearSchedule();
			}

			if(m_hPlayer != NULL && m_flNextHealTime <= gpGlobals->time 
			&& pev->sequence != LookupActivity ( ACT_RANGE_ATTACK1 )
			&& pev->sequence != LookupActivity ( ACT_RANGE_ATTACK2 )
			&& pev->sequence != LookupActivity ( ACT_HOP )
			&& pev->sequence != LookupActivity ( ACT_LEAP )
			&& !m_fHealPlayer && !m_fHealMyself){//�������
				m_PlayerHealth = m_hPlayer->pev->health;
				if(m_hPlayer->pev->deadflag == DEAD_NO && 
				m_PlayerHealth <= m_hPlayer->pev->max_health * 0.8){//��������ˣ���Ҫ����!
					if(m_hEnemy->IsPlayer()){
						float flDist = ( m_hPlayer->pev->origin - pev->origin).Length();
						if(flDist <= 128){
						m_fHealPlayer = TRUE;
						m_igonre_npc = 10;
						}
					}
				}
			}
			
		}
	}

	if(m_hEnemy != NULL && m_FTSmod == 4){
			if(m_cleardally >= 18){
				if(m_enemyfollower == 1){
				m_enemyfollower = 0;
				m_enemyfollower_combat = 0;
				}
			}
			else if(m_enemyfollower == 0){//���û���ϣ�ȥ������
				m_enemyfollower = 1;
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
		    m_flGroundSpeed = 100;
		}
		else if(pev->sequence == LookupActivity ( ACT_WALK_HURT )){
			m_flGroundSpeed = 90;
		}

			if ( pev->movetype == MOVETYPE_TOSS && !m_groundElev )
			{
				if (pev->flags & FL_ONGROUND)
				{
					pev->movetype = MOVETYPE_STEP;
				}
			}
		
	//���ܤν���
	if(m_rpgms_level >= 75 && m_rpgms_skill8_learn == 0){
		m_rpgms_skill5_learn = 13;//ħ��һ��
		m_rpgms_skill8_learn = 52;//ħ��ӻ�
		m_rpgms_skill9_learn = 78;//��������
		m_rpgms_skill10_learn = 14;//�Ҳ���
		pev->frags = 2;//����Ҳ����Ҫ
	}
	if(m_rpgms_level >= 40 && m_rpgms_skill7_learn == 0){
		m_rpgms_skill7_learn = 43;//�����ɳ�
		pev->max_health = 360;
		pev->health = pev->max_health;
	}
	if(m_rpgms_level >= 25 && m_rpgms_skill6_learn == 0){
		m_rpgms_skill6_learn = 77;//ǿ��������
	}
	
}


void CMisaliya::Killed( entvars_t *pevAttacker, int iGib )
{
	if(m_die == 0){

		if(pev->skin != 1){
		pev->skin = 4;
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
void CMisaliya :: GibMonster ( void )
{
	CBaseMonster :: GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human grunts because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CMisaliya :: ISoundMask ( void )
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
BOOL CMisaliya :: FCanCheckAttacks ( void )
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
BOOL CMisaliya :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	float cover_dist = 270;

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

	if(flDist <= cover_dist && flDot >= 0.8 && m_flNextIcetip <= gpGlobals->time ){
			if(pev->sequence != LookupActivity ( ACT_LEAP )){
			SetActivity ( ACT_LEAP );
			}
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
BOOL CMisaliya :: CheckRangeAttack1 ( float flDot, float flDist )
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
BOOL CMisaliya :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if ( m_rpgms_skill5_learn == 0 )//δѧ�Ἴ�ܡ�ħ��һ��
	{
		return FALSE;
	}

	if ( m_hEnemy == NULL || m_flNextBlastSkillTime > gpGlobals->time )
	{
		return FALSE;
	}
	else if ( m_hEnemy->IsPlayer() && m_lovehate > 0 )
	{
		return FALSE;
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

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z
	&& !FClassnameIs( m_hEnemy->pev, "npc_attack_flag" ) && !FClassnameIs( m_hEnemy->pev, "hydra_spore_fun" ) )
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


	float cover_dist = 256;

	if ( ( vecTarget - pev->origin ).Length2D() <= cover_dist )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}
	else if ( ( vecTarget - pev->origin ).Length2D() >= 2048 )
	{
		// too far
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}	

		Vector vecToss = VecCheckThrow( pev, GetGunPosition(), vecTarget, 800, 0.4 );

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 0.2;
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1;
		}
	

	return m_fThrowGrenade;
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CMisaliya :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int CMisaliya :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( (pev->spawnflags & SF_MONSTER_GAG) ){
	pev->sequence = LookupSequence( "protect_kadoma3" );
	ResetSequenceInfo( );
	pev->frame = 0;
	pev->skin = 4;
	return 0;
	}

	m_alert	= 100;

	//ħ������
	if ( (bitsDamageType & DMG_SHOCK) || (bitsDamageType & DMG_ENERGYBLAST) 
	|| (bitsDamageType & DMG_FREEZE) || (bitsDamageType & DMG_BURN)){
		flDamage *= 0.2;
	}
	else if ( (bitsDamageType & DMG_ENERGYBEAM)
	|| (bitsDamageType & DMG_DARK)
	|| (bitsDamageType & DMG_SONIC)){
		flDamage *= 0.8;
	}

	if(m_rpgms_skill8_learn == 52 && m_flNextDefine < gpGlobals->time && mps_ent == NULL
	&& flDamage >= pev->health){
		mps_ent = Create( "magic_protec_shield", pev->origin, pev->angles, edict());
		mps_ent->pev->movetype = MOVETYPE_FOLLOW;
		mps_ent->pev->aiment = edict();

		if(m_rpgms_skill10_learn == 14 && RANDOM_LONG(0,100) > 50){
		m_flNextDefine = gpGlobals->time + 30;
		}
		else{
		m_flNextDefine = gpGlobals->time + 60;
		}
		m_flNPC_Pain = 0;

		if (pevAttacker)
		{
			CBaseEntity *pEntity = GetClassPtr((CBaseEntity *)pevAttacker);

			if(pEntity->pev->deadflag == DEAD_NO && pEntity->pev->takedamage){
			pEntity->TakeDamage ( pEntity->pev, pev, flDamage, DMG_GENERIC | DMG_NEVERGIB );//����!
			}
		}
		return 0;
	}

	if(mps_ent){
		if ( pev->takedamage && pev->deadflag == DEAD_NO){
			//ħ��ӻ����������������˺�!
			if (pevAttacker)
			{
				CBaseEntity *pEntity = GetClassPtr((CBaseEntity *)pevAttacker);

				if(pEntity->pev->deadflag == DEAD_NO && pEntity->pev->takedamage){
				pEntity->TakeDamage ( pEntity->pev, pev, flDamage, DMG_GENERIC | DMG_NEVERGIB );//����!
				}
			}
			return 0;
		}
	}

	if(m_fHealPlayer || m_fHealMyself){//���Ƽ���ʹ��ʱ����!
		flDamage *= 0.5;
	}

	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CMisaliya :: SetYawSpeed ( void )
{
	pev->yaw_speed = 240;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CMisaliya :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CMisaliya :: GetGunPosition( )
{
	return pev->origin + Vector( 0, 0, 60 );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMisaliya :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case 5:
		{
			ClearSchedule();
			SetYawSpeed();
		}
		break;

		case 2:
			if(m_flNextIcetip <= gpGlobals->time && FBitSet (pev->flags, FL_ONGROUND )){
			pev->movetype = MOVETYPE_TOSS;
			pev->flags &= ~FL_ONGROUND;
			UTIL_MakeVectors(pev->angles);
			pev->velocity = gpGlobals->v_forward * -512;
			pev->velocity.z += 128;

					Vector ice_org;
					ice_org = pev->origin + gpGlobals->v_forward * 64;
					if(m_hEnemy != NULL){
						if(FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND )){
							if(( pev->origin - m_hEnemy->pev->origin).Length() <= 384){
							ice_org = m_hEnemy->pev->origin;
							m_hEnemy->TakeDamage( pev, pev, 60, DMG_FREEZE | DMG_CONCUSSION);
							}
						}
					}

					CBaseEntity::Create( "ice_tip", ice_org, pev->angles, edict() );
					m_flNextIcetip = gpGlobals->time + 4;

					if(m_rpgms_skill10_learn == 14 && RANDOM_LONG(0,100) > 50){
					m_flNextIcetip = gpGlobals->time + 2;
					}
			}
		break;

		case 1:
		{
			if(m_flNextShootTime <= gpGlobals->time && m_hEnemy != NULL){
			Vector	vecSpitOffset,vangle;
			Vector	vecSpitDir;

			UTIL_MakeVectors ( pev->angles );

			// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
			// we should be able to read the position of bones at runtime for this info.
			GetAttachment( 0, vecSpitOffset, vangle );

			vecSpitDir = ( m_hEnemy->Center() - vecSpitOffset ).Normalize();

			m_flNextShootTime = gpGlobals->time + 2;
			if(m_rpgms_skill10_learn == 14 && RANDOM_LONG(0,100) > 50){
			m_flNextShootTime = gpGlobals->time + 1;
			}

			if(m_flNextShoot2Time <= gpGlobals->time && m_rpgms_skill6_learn == 77){//ǿ��������!
				m_flNextShoot2Time = gpGlobals->time + 8;
				if(m_rpgms_skill10_learn == 14 && RANDOM_LONG(0,100) > 50){
				m_flNextShoot2Time = gpGlobals->time + 4;
				}
			CShockBall::Shoot( pev, vecSpitOffset, vecSpitDir * 2000,2 );
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/shock_fire.wav", 0.8, ATTN_NORM,0,100 + RANDOM_LONG(5,15) );
			}
			else{
			CShockBall::Shoot( pev, vecSpitOffset, vecSpitDir * 2000,1 );
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/shock_fire.wav", 0.7, ATTN_NORM,0,100 + RANDOM_LONG(-5,5) );
			}

			}
			else{
				if(m_hEnemy == NULL || (m_hEnemy->IsPlayer() && m_lovehate > 0) ){
				ClearSchedule();
				SetYawSpeed();
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

		case 3:
		{
			if(m_fHealMyself){//����
				if(m_flNextHealTime <= gpGlobals->time){
					if ( IsAlive() ){
						Vector	vecSpitOffset,vangle;
						GetAttachment( 0, vecSpitOffset, vangle );

						m_flNextHealTime = gpGlobals->time + 50;

						if(m_rpgms_skill10_learn == 14 && RANDOM_LONG(0,100) > 50){
						m_flNextHealTime = gpGlobals->time + 25;
						}

						TakeHealth(pev->max_health * 0.2, DMG_GENERIC);
						FX_Explosion( vecSpitOffset, 101);
						FX_Explosion( Center(), EXPLOSION_MEDKIT);
					}
				}
			}
			else if(m_hPlayer != NULL && m_flNextHealTime <= gpGlobals->time){//�������
				m_flNextHealTime = gpGlobals->time + 4;
				if ( m_hPlayer->IsAlive() ){
					Vector	vecSpitOffset,vangle;
					GetAttachment( 0, vecSpitOffset, vangle );

					float flDist = ( m_hPlayer->pev->origin - pev->origin).Length();
					if(flDist <= 160 && FInViewCone3 ( m_hPlayer )){
						m_flNextHealTime = gpGlobals->time + 50;

						if(m_rpgms_skill10_learn == 14 && RANDOM_LONG(0,100) > 50){
						m_flNextHealTime = gpGlobals->time + 25;
						}

						m_hPlayer->TakeHealth(m_hPlayer->pev->max_health * 0.2, DMG_GENERIC);
						FX_Explosion( vecSpitOffset, 101);
						FX_Explosion( m_hPlayer->Center(), EXPLOSION_MEDKIT);
					}
				}
			}

			m_fHealPlayer = FALSE;
			m_fHealMyself = FALSE;
		}
		break;

		case 6:
		{
		Vector	vecSpitOffset,vangle;
		GetAttachment( 0, vecSpitOffset, vangle );
		FX_Explosion( vecSpitOffset, 101);
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_kadoma" );
				if ( pEntity )//��Kadoma����
				{
					FX_Explosion( pEntity->pev->origin, EXPLOSION_MEDKIT);
				}
		}
		break;

		case 7:
		{//ʹ��ħ��ˮ��
			Vector	vecSpitOffset,vangle;
			GetAttachment( 0, vecSpitOffset, vangle );
			FX_Explosion( vecSpitOffset, 103);

			CBaseEntity *pEntity = NULL;
			while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 128 )) != NULL)
			{
				if ( FClassnameIs( pEntity->pev, "xen_unknow_light" ) ){
					pEntity->pev->frags = 1;
					break;
				}
			}
		}
		break;

		case 8:
		{
			if(m_flNextBlastSkillTime <= gpGlobals->time){
				m_listenlong = 20;

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

				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/gauss_fire2.wav", 1, ATTN_NORM);
				CGrenade::ShootContact_low( pev, GetGunPosition(), m_vecTossVelocity );
				m_fThrowGrenade = FALSE;
				
				m_flNextBlastSkillTime = gpGlobals->time + 10;
				
				if(m_rpgms_skill10_learn == 14 && RANDOM_LONG(0,100) > 50){
				m_flNextBlastSkillTime = gpGlobals->time + 5;
				}
			}
			else{
				ClearSchedule();
				SetYawSpeed();
			}
		}
		break;

		case 9:
		{//����HUNT
			SetState( MONSTERSTATE_HUNT );
	
		}
		break;

		case 10:
		{//ħ����ȡ����
			//SetBodygroup( 9, 1 );
			//FX_Explosion( Center(), 42);
		}
		break;

		case 11:
		{//����
		//	m_die = 0;
		//	m_dieseq	= 0;
		//	pev->health = pev->max_health;
		//	pev->deadflag = DEAD_NO;
		//	SetActivity( ACT_IDLE );
		//	SetState( MONSTERSTATE_IDLE );
		//	ClearSchedule();
		//	SetYawSpeed();

			FX_Explosion( Center(), 102);
			pev->effects |= EF_NODRAW;
			pev->takedamage = DAMAGE_NO;
			pev->solid		= SOLID_NOT;
		}
		break;

		case 12://��������
			if(m_flNextIceBreak <= gpGlobals->time && m_hEnemy != NULL
			&& m_rpgms_skill9_learn == 78 && FBitSet (pev->flags, FL_ONGROUND )){

				Vector ice_org;
				ice_org = pev->origin;
				if(m_hEnemy != NULL){
						if(( pev->origin - m_hEnemy->pev->origin).Length() <= 2048){
						ice_org = m_hEnemy->pev->origin;
						m_hEnemy->TakeDamage( pev, pev, 300, DMG_FREEZE );

							CBaseMonster *pEnemyMonster;
							pEnemyMonster = m_hEnemy->MyMonsterPointer();
							if(pEnemyMonster){
							pEnemyMonster->Freeze_Monster(60);
							}

						}
				}
				
				CBaseEntity::Create( "ice_break", ice_org, pev->angles, edict() );
				m_flNextIceBreak = gpGlobals->time + 42;

				if(m_rpgms_skill10_learn == 14 && RANDOM_LONG(0,100) > 50){
				m_flNextIceBreak = gpGlobals->time + 21;
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
void CMisaliya :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/misaliya.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;

	pev->health			= 240;//Ů���δ�Ƥ
	m_lovehate			= 30;

	m_flFieldOfView		 = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		 = MONSTERSTATE_NONE;

	m_flNextPainTime	 = gpGlobals->time;
	m_flNextBlastSkillTime = gpGlobals->time + 2;
	m_flNextShootTime    = gpGlobals->time + 2;
	m_flNextShoot2Time   = gpGlobals->time + 2;
	m_flNextHealTime     = gpGlobals->time + 2;
	m_flNextGrenadeCheck = gpGlobals->time + 2;
	m_flNextIcetip		 = gpGlobals->time + 4;
	m_flNextDefine		 = gpGlobals->time + 4;

	m_afCapability		= bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;

	m_HackedGunPos = Vector ( 0, 0, 55 );

	m_fHealPlayer = FALSE;
	m_fHealMyself = FALSE;

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

	m_follow_mode = 1;
	m_ignoredamage		= 1;
	m_headdef			= 2;

	m_candrownwater = 1;
	m_forcefuckdoor = TRUE;
	m_chase_mode = 2;
	m_chase_failed_max = 4;

	SetUse( &CMisaliya::FollowerUse2 );

	pev->body = 0;
	SetTouch( &CMisaliya::DeadTouch );

	m_rpgms_actor = 12;
	m_rpgms_level = 20;
	m_rpgms_exp = 0;
	m_rpgms_type = 1;

	m_new_ally_type = TRUE;
	pev->netname = MAKE_STRING( "Misaliya" );

	m_rpgms_skill1_learn = 10;//������
	m_rpgms_skill2_learn = 11;//��׶
	m_rpgms_skill3_learn = 12;//����
	m_rpgms_skill4_learn = 38;//ħ������
	m_rpgms_skill5_learn = 0;
	m_rpgms_skill6_learn = 0;
	m_rpgms_skill7_learn = 0;
	m_rpgms_skill8_learn = 0;
	m_rpgms_skill9_learn = 0;
	m_rpgms_skill10_learn = 0;

	pev->takedamage = DAMAGE_YES;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMisaliya :: Precache()
{
	PRECACHE_MODEL("models/misaliya.mdl");
	PRECACHE_MODEL("models/ice_tip.mdl");
	PRECACHE_MODEL("models/mfz.mdl");
	PRECACHE_MODEL("models/magic_protec2.mdl");

	PRECACHE_SOUND("misaliya/pain1.wav" );
	PRECACHE_SOUND("misaliya/pain2.wav" );
//	PRECACHE_SOUND("misaliya/pain3.wav" );
//	PRECACHE_SOUND("misaliya/die1.wav" );
	PRECACHE_SOUND("misaliya/die2.wav" );

	PRECACHE_SOUND("misaliya/frostnova.wav" );
	PRECACHE_SOUND("war3/impalehit.wav" );
	PRECACHE_SOUND("misaliya/frostnova2.wav" );

	PRECACHE_MODEL("sprites/b-tele1.spr");

	PRECACHE_SOUND("weapons/shock_fire.wav" );
	PRECACHE_SOUND("weapons/shock_blast.wav" );

	PRECACHE_SOUND("weapons/gauss_fire2.wav" );
	PRECACHE_SOUND("weapons/gluon_hitwall2.wav" );
	
//	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event
}	

//=========================================================
// start task
//=========================================================
void CMisaliya :: StartTask ( Task_t *pTask )
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
void CMisaliya :: RunTask ( Task_t *pTask )
{
	CSquadMonster :: RunTask( pTask );
}

//=========================================================
// PainSound
//=========================================================
void CMisaliya :: PainSound ( void )
{
	if ( pev->spawnflags & SF_MONSTER_GAG ){
	return;
	}

	if ( gpGlobals->time > m_flNextPainTime )
	{

		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	
			EMIT_SOUND_DYN( ENT(pev), 6, "misaliya/pain1.wav", 1, 0.7, 0, 100);
			break;
		case 1:
			EMIT_SOUND_DYN( ENT(pev), 6, "misaliya/pain2.wav", 1, 0.7, 0, 100);	
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CMisaliya :: DeathSound ( void )
{
	if ( pev->spawnflags & SF_MONSTER_GAG ){
	return;
	}

	EMIT_SOUND_DYN( ENT(pev), 6, "misaliya/die2.wav", 1, 0.7, 0, 100);

	//switch ( RANDOM_LONG(0,1) )
//	{
	//	case 0:	
	//		EMIT_SOUND( ENT(pev), 6, "misaliya/die1.wav", 1, 0.7 );		
	//		break;
	//	case 1:
//			EMIT_SOUND( ENT(pev), 6, "misaliya/die2.wav", 1, 0.7 );	
	//		break;
//	}
}

//=========================================================
// GruntFail
//=========================================================
Task_t	tlMISAFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)1		},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slMISAFail[] =
{
	{
		tlMISAFail,
		ARRAYSIZE ( tlMISAFail ),
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
Task_t	tlMISACombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)1		},
};

Schedule_t	slMISACombatFail[] =
{
	{
		tlMISACombatFail,
		ARRAYSIZE ( tlMISACombatFail ),
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
Task_t	tlMISAVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
};

Schedule_t	slMISAVictoryDance[] =
{
	{ 
		tlMISAVictoryDance,
		ARRAYSIZE ( tlMISAVictoryDance ), 
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
Task_t tlMISAEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_GRUNT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slMISAEstablishLineOfFire[] =
{
	{ 
		tlMISAEstablishLineOfFire,
		ARRAYSIZE ( tlMISAEstablishLineOfFire ),
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
Task_t	tlMISAFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slMISAFoundEnemy[] =
{
	{ 
		tlMISAFoundEnemy,
		ARRAYSIZE ( tlMISAFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"GruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlMISACombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_COMBAT_IDLE		},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)0.5					},
};

Schedule_t	slMISACombatFace[] =
{
	{ 
		tlMISACombatFace1,
		ARRAYSIZE ( tlMISACombatFace1 ), 
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2		|
		bits_COND_CAN_MELEE_ATTACK1,
		0,
		"Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t	tlMISASignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slMISASignalSuppress[] =
{
	{ 
		tlMISASignalSuppress,
		ARRAYSIZE ( tlMISASignalSuppress ), 
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

Task_t	tlMISASuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slMISASuppress[] =
{
	{ 
		tlMISASuppress,
		ARRAYSIZE ( tlMISASuppress ), 
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
Task_t	tlMISAWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slMISAWaitInCover[] =
{
	{ 
		tlMISAWaitInCover,
		ARRAYSIZE ( tlMISAWaitInCover ), 
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
Task_t	tlMISATakeCover1[] =
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

Schedule_t	slMISATakeCover[] =
{
	{ 
		tlMISATakeCover1,
		ARRAYSIZE ( tlMISATakeCover1 ), 
		bits_COND_ENEMY_DEAD,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlMISAGrenadeCover1[] =
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

Schedule_t	slMISAGrenadeCover[] =
{
	{ 
		tlMISAGrenadeCover1,
		ARRAYSIZE ( tlMISAGrenadeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};


//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlMISATossGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slMISATossGrenadeCover[] =
{
	{ 
		tlMISATossGrenadeCover1,
		ARRAYSIZE ( tlMISATossGrenadeCover1 ), 
		0,
		0,
		"TossGrenadeCover"
	},
};
//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlMISATakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slMISATakeCoverFromBestSound[] =
{
	{ 
		tlMISATakeCoverFromBestSound,
		ARRAYSIZE ( tlMISATakeCoverFromBestSound ), 
		0,
		0,
		"TakeCoverFromBestSound"
	},
};


//=========================================================
// Grunt reload schedule
//=========================================================
Task_t	tlMISAHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_USE				},
};

Schedule_t slMISAHideReload[] = 
{
	{
		tlMISAHideReload,
		ARRAYSIZE ( tlMISAHideReload ),
		0,
		0,
		"Misaliya Heal"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlMISASweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_FORGET_ENEMY,		(float)0	},
};

Schedule_t	slMISASweep[] =
{
	{ 
		tlMISASweep,
		ARRAYSIZE ( tlMISASweep ), 
		
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
Task_t	tlMISARangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
};

Schedule_t	slMISARangeAttack1A[] =
{
	{ 
		tlMISARangeAttack1A,
		ARRAYSIZE ( tlMISARangeAttack1A ), 
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
Task_t	tlMISARangeAttack1B[] =
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

Schedule_t	slMISARangeAttack1B[] =
{
	{ 
		tlMISARangeAttack1B,
		ARRAYSIZE ( tlMISARangeAttack1B ), 
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
Task_t	tlMISARangeAttack2[] =
{
	{ TASK_STOP_MOVING,						(float)0							},
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
};

Schedule_t	slMISARangeAttack2[] =
{
	{ 
		tlMISARangeAttack2,
		ARRAYSIZE ( tlMISARangeAttack2 ), 
		0,
		0,
		"RangeAttack2"
	},
};


DEFINE_CUSTOM_SCHEDULES( CMisaliya )
{
	slMISAFail,
	slMISACombatFail,
	slMISAVictoryDance,
	slMISAEstablishLineOfFire,
	slMISAFoundEnemy,
	slMISACombatFace,
	slMISASignalSuppress,
	slMISASuppress,
	slMISAWaitInCover,
	slMISATakeCover,
	slMISAGrenadeCover,
	slMISATossGrenadeCover,
	slMISATakeCoverFromBestSound,
	slMISAHideReload,
	slMISASweep,
	slMISARangeAttack1A,
	slMISARangeAttack1B,
	slMISARangeAttack2,
};

IMPLEMENT_CUSTOM_SCHEDULES( CMisaliya, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void CMisaliya :: SetActivity ( Activity NewActivity )
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
		if ( pev->health <= pev->max_health * 0.5 )
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
		if ( pev->health <= pev->max_health * 0.5 )
		{
			NewActivity = ACT_GUARD;
		}
		else if ( m_MonsterState == MONSTERSTATE_COMBAT && m_hEnemy != NULL )
		{
			NewActivity = ACT_STAND;
		}
		iSequence = LookupActivity ( NewActivity );
		break;
	case ACT_RANGE_ATTACK2:
		iSequence = LookupSequence( "skill" );
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
Schedule_t *CMisaliya :: GetSchedule( void )
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
						if(m_flNextShootTime > gpGlobals->time){
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
							return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
							}
						}
					}
				}
	// can kick
				else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
				{
					return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
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
				else if ( m_fHealPlayer || m_fHealMyself )
				{
					//!!!KELLY - this individual just realized he's out of bullet ammo. 
					// He's going to try to find cover to run to and reload, but rarely, if 
					// none is available, he'll drop and reload in the open here. 
					return GetScheduleOfType ( SCHED_GRUNT_COVER_AND_RELOAD );
				}
	// can't see enemy
				else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
				{
					if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ))
					{
						return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
					}
					else
					{
						return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
					}
				}
				
				if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
				{
						if(m_flNextShootTime > gpGlobals->time){
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
Schedule_t* CMisaliya :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			return &slMISATakeCover[ 0 ];
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slMISATakeCoverFromBestSound[ 0 ];
		}
	case SCHED_GRUNT_TAKECOVER_FAILED:
		{
			if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}
			else if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ))
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
			return &slMISACombatFace[ 0 ];
			}
			else{
			return GetScheduleOfType ( SCHED_CHASE_ENEMY_FAILED );
			}
		}
		break;
	case SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			if(m_groundElev){
			return &slMISACombatFace[ 0 ];
			}
			else{
			return &slMISAEstablishLineOfFire[ 0 ];
			}
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			return &slMISARangeAttack1A[ 0 ];
		}
	case SCHED_RANGE_ATTACK2:
		{
			return &slMISARangeAttack2[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slMISACombatFace[ 0 ];
		}
	case SCHED_GRUNT_WAIT_FACE_ENEMY:
		{
			return &slMISAWaitInCover[ 0 ];
		}
	case SCHED_GRUNT_SWEEP:
		{
			return &slMISASweep[ 0 ];
		}
	case SCHED_GRUNT_COVER_AND_RELOAD:
		{
			return &slMISAHideReload[ 0 ];
		}
	case SCHED_GRUNT_FOUND_ENEMY:
		{
			return &slMISAFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			return &slMISAVictoryDance[ 0 ];
		}
	case SCHED_GRUNT_SUPPRESS:
		{
			return &slMISASuppress[ 0 ];
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slMISACombatFail[ 0 ];
			}

			return &slMISAFail[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}