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
// Zombie
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"animation.h"

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CBarnacleDemon : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void EXPORT LeapTouch ( CBaseEntity *pOther );

	float m_flNextFlinch;
	float m_flNextPainTime;

	void Killed( entvars_t *pevAttacker, int iGib );

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );


	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	void SetActivity ( Activity NewActivity );

	void RunAI( void );

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist );//ǣ��
	BOOL CheckRangeAttack2 ( float flDot, float flDist );//�ٻ�
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );//�չ�
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );//����
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	Vector m_teleportorigin;

	EHANDLE	m_childguy;

	float m_SkillTime1;
	float m_SkillTime2;
	float m_SkillTime3;
	float m_flKillBeamTime;
	float m_RadiusmodeTime;
	float m_SuckmodeTime;
};

LINK_ENTITY_TO_CLASS( monster_barnacle_boss, CBarnacleDemon );

TYPEDESCRIPTION	CBarnacleDemon::m_SaveData[] = 
{
	DEFINE_FIELD( CBarnacleDemon, m_SkillTime1, FIELD_TIME ),
	DEFINE_FIELD( CBarnacleDemon, m_SkillTime2, FIELD_TIME ),
	DEFINE_FIELD( CBarnacleDemon, m_SkillTime3, FIELD_TIME ),
	DEFINE_FIELD( CBarnacleDemon, m_RadiusmodeTime, FIELD_TIME ),
	DEFINE_FIELD( CBarnacleDemon, m_SuckmodeTime, FIELD_TIME ),
	DEFINE_FIELD( CBarnacleDemon, m_flKillBeamTime, FIELD_TIME ),
	DEFINE_FIELD( CBarnacleDemon, m_teleportorigin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBarnacleDemon, m_childguy, FIELD_EHANDLE ),
//	DEFINE_FIELD( CBarnacleDemon, m_hEnemyBite, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CBarnacleDemon, CBaseMonster );

// ����Ŀ��
void MovetoTarget_2( entvars_t *pevFucker, Vector vecTarget,float speed )
{
	// accelerate
	Vector m_vecIdeal;
	m_vecIdeal = Vector( 0, 0, 0 );
	float flSpeed = m_vecIdeal.Length();
	if (flSpeed == 0)
	{
		m_vecIdeal = pevFucker->velocity;
		flSpeed = m_vecIdeal.Length();
	}

	if (flSpeed > (speed + 768))
	{
		m_vecIdeal = m_vecIdeal.Normalize( ) * (speed + 768);
	}

	m_vecIdeal = m_vecIdeal + (vecTarget - pevFucker->origin).Normalize() * speed;

	pevFucker->velocity = m_vecIdeal;
}

void CBarnacleDemon :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->sequence == LookupActivity ( ACT_IDLE )){
		if(m_SuckmodeTime > 0){
		m_SuckmodeTime = 0;
		}
	}
	if(pev->sequence == LookupActivity ( ACT_WALK )){
		m_flGroundSpeed = 200;
	}
	if(pev->sequence != LookupActivity ( ACT_MELEE_ATTACK2 )){
		if(pev->movetype == MOVETYPE_BOUNCEMISSILE){
		pev->movetype = MOVETYPE_STEP;
		m_groundElev2 = FALSE;
		pev->velocity.x = 0;
		pev->velocity.y = 0;
		m_RadiusmodeTime = 0;
		SetTouch ( NULL );
		m_SuckmodeTime = 0;
		}
	}

	if(m_SuckmodeTime > gpGlobals->time){
		if(m_hEnemy != NULL){

			if(m_hEnemy->pev->deadflag != DEAD_NO
			|| FBitSet( m_hEnemy->pev->flags, FL_NOTARGET )){
			m_SuckmodeTime = 0;//Bug Fix 3.0 �޸��ٺ�BOSS�����ж�
			}

					MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
						WRITE_BYTE( TE_BEAMENTS );
						WRITE_SHORT( entindex() + 0x1000 );
						WRITE_SHORT( m_hEnemy->entindex() );
						WRITE_SHORT( g_sModelIndexTrail );
						WRITE_BYTE( 0 ); // framestart
						WRITE_BYTE( 0 ); // framerate
						WRITE_BYTE( 10 ); // life
						WRITE_BYTE( 30 );  // width
						WRITE_BYTE( 0 );   // noise
						WRITE_BYTE( 255 );   // r, g, b
						WRITE_BYTE( 32 );   // r, g, b
						WRITE_BYTE( 32 );   // r, g, b
						WRITE_BYTE( 255 );	// brightness
						WRITE_BYTE( 0 );		// speed
					MESSAGE_END();

					MovetoTarget_2( m_hEnemy->pev,pev->origin + Vector(0,0,48),450);

					Vector vecGunPos,vecGunAngles;
					GetAttachment( 0, vecGunPos, vecGunAngles );

					if(( pev->origin - m_hEnemy->pev->origin).Length() <= 96){
						if(m_hEnemy->pev->takedamage){
						m_hEnemy->TakeDamage ( pev, pev, 25, DMG_GENERIC | DMG_CONCUSSION);//����ѣ��!
						}
					}
		}
	}

	if(m_RadiusmodeTime > gpGlobals->time){
		if(m_freezetime > 0){
		m_RadiusmodeTime = 0;
		}
		if(pev->velocity.Length() <= 800){
			pev->velocity.x += RANDOM_LONG(-1200,1200);
			pev->velocity.y += RANDOM_LONG(-1200,1200);
		}
		pev->velocity.z = 0;
		::RadiusDamage_limit( Center(), pev, pev, 64, 192, CLASS_HUMAN_ASS, DMG_GENERIC);
	}

	if(m_flKillBeamTime > 0 && m_flKillBeamTime < gpGlobals->time){
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
	MESSAGE_END();
	m_flKillBeamTime = 0;
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CBarnacleDemon :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

//=========================================================
// �ٻ�
//=========================================================
BOOL CBarnacleDemon::CheckRangeAttack2( float flDot, float flDist )
{
	if ( gpGlobals->time > m_SkillTime1 && m_childguy == NULL && pev->health < pev->max_health * 0.9)//��������
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// ǣ��
//=========================================================
BOOL CBarnacleDemon::CheckRangeAttack1( float flDot, float flDist )
{
	if ( gpGlobals->time > m_SkillTime2 && pev->health < pev->max_health * 0.8)
	{
		return TRUE;
	}
	return FALSE;
}


BOOL CBarnacleDemon :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	float dist = 80;

	if (flDist <= dist && m_hEnemy != NULL && flDot >= 0.5 && !HasConditions(bits_COND_CAN_MELEE_ATTACK2))
	{
		return TRUE;
	}

	return FALSE;
}

//����
BOOL CBarnacleDemon :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	if(m_SkillTime3 <= gpGlobals->time && m_hEnemy != NULL && pev->health < pev->max_health * 0.9){
		m_facing_fucking_mode = 1;
		return TRUE;
	}

	m_facing_fucking_mode = 0;
	return FALSE;
}


void CBarnacleDemon::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

Schedule_t* CBarnacleDemon :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CBarnacleDemon :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBarnacleDemon :: SetYawSpeed ( void )
{
	pev->yaw_speed = 120;
}

int CBarnacleDemon :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	m_alert	= 100;

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CBarnacleDemon::Killed( entvars_t *pevAttacker, int iGib )
{
	if(m_childguy != NULL){
		if(m_childguy->pev->deadflag == DEAD_NO){
		m_childguy->Killed( pev, GIB_NEVER );
		}
	}
	m_RadiusmodeTime = 0;
	SetTouch ( NULL );
	m_SuckmodeTime = 0;
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}


//=========================================================
// SetActivity 
//=========================================================
void CBarnacleDemon :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RUN:
		iSequence = LookupActivity ( NewActivity );
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
	//	pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

void CBarnacleDemon :: LeapTouch ( CBaseEntity *pOther )
{
	int dmg;
	dmg			= 80;

	if(m_RadiusmodeTime < 0){
		return;
	}

	if ( !pOther->pev->takedamage || pev->deadflag != DEAD_NO){
		return;
	}

	if(pev->velocity.Length() < 600){
		return;
	}

	if ( pOther->Classify() == Classify() ){
		return;
	}
	
	if ( pOther->pev->flags & (FL_MONSTER|FL_CLIENT) ){
	pOther->TakeDamage( pev, pev, dmg, DMG_GENERIC);
	pOther->pev->velocity = pOther->pev->velocity + (pOther->pev->origin - pev->origin).Normalize() * 400;
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CBarnacleDemon :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;

	dmg = 80;

	switch( pEvent->event )
	{
		case 1:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_z(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 100;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_GENERIC); 
			ApplyMultiDamage( pev, pev );

				if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) ){
				pEntity->pev->velocity = pEntity->pev->velocity + (pEntity->pev->origin - pev->origin).Normalize() * 200;
				}

			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "tyant_boss/slash.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 80, dmg, DMG_GENERIC);
				if(pHurt){
					if (pHurt->Classify() == CLASS_PLAYER || pHurt->Classify() == CLASS_PLAYER_ALLY
					|| pHurt->Classify() == CLASS_HUMAN_ASS || pHurt->Classify() == CLASS_HUMAN_PASSIVE
					|| pHurt->Classify() == CLASS_HUMAN_MILITARY){
					FX_Explosion( pHurt->Center(), 234 );
					}
					else if (pHurt->Classify() == CLASS_ALIEN_MONSTER || pHurt->Classify() == CLASS_ALIEN_MILITARY){
					FX_Explosion( pHurt->Center(), 235 );
					}

					if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) ){
					pEntity->pev->velocity = pEntity->pev->velocity + (pEntity->pev->origin - pev->origin).Normalize() * 200;
					}

				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "tyant_boss/slash.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else if(m_hEnemy != NULL){
					if(( pev->origin - m_hEnemy->pev->origin).Length() <= 100){

						if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) ){
						pEntity->pev->velocity = pEntity->pev->velocity + (pEntity->pev->origin - pev->origin).Normalize() * 200;
						}

					m_hEnemy->TakeDamage( pev, pev, dmg, DMG_GENERIC);
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "tyant_boss/slash.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
			}
			
		}
		break;

		case 2:
		{
			CBaseMonster *pEnemy;
			pEnemy = m_hEnemy->MyMonsterPointer();
			if ( !pEnemy )
			{
				m_SuckmodeTime = 0;
				m_SkillTime2 = gpGlobals->time + 6.0;
			}
			else if(pEnemy->m_MonsterState == MONSTERSTATE_PRONE 
			|| pEnemy->m_IdealMonsterState == MONSTERSTATE_PRONE){
				m_SuckmodeTime = 0;
				m_SkillTime2 = gpGlobals->time + 6.0;
			}
			else{
				m_SkillTime2 = gpGlobals->time + 24.0;
				m_SuckmodeTime = gpGlobals->time + 6.0;
			}
			m_RadiusmodeTime = 0;
		}
		break;

		case 3:
		{
			m_SuckmodeTime = 0;
		}
		break;

		case 4:
		{
			if(m_childguy == NULL){
			CBaseMonster *pEnemyMonster;
			CBaseEntity *pMonsterEntity;
			pMonsterEntity = Create( "monster_barnacle_fantasy_r", pev->origin + Vector(0,0,384), pev->angles, edict() );
			pEnemyMonster = pMonsterEntity->MyMonsterPointer();
			pEnemyMonster->m_fightmode = 2;
			pEnemyMonster->m_killed_exp = 1;//����û�о���ֵ!
			m_childguy = pMonsterEntity;

			UTIL_MakeVectors(pev->angles);
			pMonsterEntity->pev->velocity = gpGlobals->v_forward * 300;
			}
			ClearSchedule();
			m_SkillTime1 = gpGlobals->time + 12.0;
		}
		break;

		case 5:
		{
			ClearSchedule();
		}
		break;

		case 6:
		{
			//pev->movetype = MOVETYPE_STEP;
			//pev->velocity.x = -50;
		}
		break;

		case 8:
		{
			UTIL_MakeVectors(pev->angles);
			ClearBits( pev->flags, FL_ONGROUND );
			pev->movetype = MOVETYPE_BOUNCEMISSILE;
			m_groundElev2 = TRUE;//��ֹ��������?
			pev->velocity.z = 0;
			pev->velocity = gpGlobals->v_forward * 900;
			m_SkillTime3 = gpGlobals->time + 12.0;
			m_RadiusmodeTime = gpGlobals->time + 4.0;
			m_SuckmodeTime = 0;
			SetTouch ( &CBarnacleDemon::LeapTouch );
		}
		break;

		case 9:
		{
			pev->movetype = MOVETYPE_STEP;
			m_RadiusmodeTime = 0;
			SetTouch ( NULL );
		}
		break;

		case 7://��β1+2
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail);	// model
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 6 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 32 );	// G
			WRITE_BYTE( 32 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail);	// model
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 6 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 32 );	// G
			WRITE_BYTE( 32 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			m_flKillBeamTime = gpGlobals->time + 1.0;
		}
		break;

		case 10://��β1+2����ʱ��
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail);	// model
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 6 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 32 );	// G
			WRITE_BYTE( 32 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail);	// model
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 6 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 32 );	// G
			WRITE_BYTE( 32 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			m_flKillBeamTime = gpGlobals->time + 4.0;
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CBarnacleDemon :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/barnacle_demon.mdl");
	UTIL_SetSize(pev, Vector(-32,-32,0), Vector(32,32,128));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 25000;
	}
	else{
	pev->health			= 20000;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	
	MonsterInit();

	pev->body			= 0;
	pev->gravity		= 2.0;

	m_lookignoremod = 1;

	m_SkillTime1 = 0;
	m_SkillTime2 = 0;
	m_SkillTime3 = 0;
	m_flKillBeamTime = 0;
	m_RadiusmodeTime = 0;
	m_SuckmodeTime = 0;

	m_ignoredamage = 1;

	m_selfmode = TRUE;
	m_MoveFail_SimpleRoad = TRUE;

	m_killed_exp = 3000;
	m_rpgms_level = 90;
	m_is_the_boss = TRUE;
	pev->netname = MAKE_STRING( "Barnacle.Demon" );

	m_childguy = NULL;

	m_freeze_def = 1;//����ο���LV1
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBarnacleDemon :: Precache()
{
	PRECACHE_MODEL("models/barnacle_demon.mdl");
	PRECACHE_SOUND("tyant_boss/slash.wav");
	UTIL_PrecacheOther( "monster_barnacle_fantasy_r" );
}	
