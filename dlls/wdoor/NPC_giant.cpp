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
#include	"player.h"
#include	"scripted.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CGiant : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	float m_flNextFlinch;
	float m_flNextPainTime;

	int IgnoreConditions ( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void RunAI( void );

	int IRelationship( CBaseEntity *pTarget );

	float m_dyingtime;

	void AlertSound( void );
	void DeathSound( void );
	void PainSound( void );
	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	BOOL CheckMeleeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	virtual int	ObjectCaps( void ) { return CBaseMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	void EXPORT		FollowerUse2( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	float	m_painTime;
	float	m_SkillTime;
	float	m_flNextGodTime;

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS( monster_giant, CGiant );

TYPEDESCRIPTION	CGiant::m_SaveData[] = 
{
	DEFINE_FIELD( CGiant, m_dyingtime, FIELD_TIME ),
	DEFINE_FIELD( CGiant, m_SkillTime, FIELD_TIME ),
	DEFINE_FIELD( CGiant, m_flNextGodTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CGiant, CBaseMonster );

BOOL CGiant :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	if(!IsAlive()){
	return FALSE;
	}

	float cover_dist = 90;

	if ( m_hEnemy != NULL )
	{
		if(FClassnameIs(m_hEnemy->pev, "monster_doma_boss")){
		cover_dist += 160;
		}
	}

	if (flDist <= cover_dist && flDot >= 0.5 && m_hEnemy != NULL)
	{
	return TRUE;
	}

	return FALSE;
}

BOOL CGiant :: CheckRangeAttack2 ( float flDot, float flDist )
{//�߸�һ��
	if(m_rpgms_skill4_learn == 73 && m_SkillTime < gpGlobals->time && flDist <= 1000){
	return TRUE;
	}

	return FALSE;
}


void CGiant :: FollowerUse2( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	
	if ( IsAlive() && pCaller != NULL && pCaller->IsPlayer())
	{
			if ( m_MonsterState == MONSTERSTATE_SCRIPT || m_IdealMonsterState == MONSTERSTATE_SCRIPT )
			{
				if(!m_pCine->CanInterrupt()){
					return;
				}
			}

			if(m_hEnemy != NULL){
			m_hEnemy = NULL;
			m_hOldEnemy[0] = NULL;
			m_hOldEnemy[1] = NULL;
			m_hOldEnemy[2] = NULL;
			m_hOldEnemy[3] = NULL;
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


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CGiant :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

void CGiant::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

Schedule_t* CGiant :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CGiant :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGiant :: SetYawSpeed ( void )
{
	pev->yaw_speed = 300;
}

int CGiant :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if(pev->sequence == LookupActivity ( ACT_RANGE_ATTACK2 ) 
	|| pev->sequence == LookupActivity ( ACT_GUARD )){
	flDamage *= 0.1;//�����ͷ��У��˺�������٣�
	}
	else if(m_rpgms_skill3_learn == 72){//�񵲷���
		if(pev->sequence == LookupActivity ( ACT_MELEE_ATTACK1 )){
			if(!(bitsDamageType & (DMG_AIR | DMG_FALL | DMG_DROWN | DMG_NERVEGAS))){
				if (RANDOM_LONG(0,100) <= 25){
				SetActivity ( ACT_GUARD );
				return 0;
				}
			}
		}
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CGiant::Killed( entvars_t *pevAttacker, int iGib )
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

	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

void CGiant :: AlertSound( void )
{
//	EMIT_SOUND_DYN( ENT(pev), CHAN_AUTO, "giant/alert.wav", 1, 0.6, 0, 90);
}

void CGiant :: DeathSound ( void )
{
	if(pev->weapons == 0){
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "giant/die.wav", 1, 0.6, 0, 90);
	}
	else{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "giant/pain.wav", 1, 0.6, 0, 90);
	}
}

void CGiant :: PainSound ( void )
{
	if (gpGlobals->time < m_painTime)
		return;
	
	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "giant/pain.wav", 1, 0.6, 0, 90);
}

int CGiant::IRelationship ( CBaseEntity *pTarget )
{
	return CBaseMonster::IRelationship( pTarget );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGiant :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg1 = 30;
	int dmg2 = 60;

	switch( pEvent->event )
	{
		case 1:
		{
			if(pev->body != 0){
			pev->body = 0;
			}

			if(m_rpgms_skill1_learn == 18 && RANDOM_LONG(0,100) <= 20){//����һ��
			dmg1 *= 3;
			}

			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 100;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg1, gpGlobals->v_forward, &tr, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );


				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );


			}
			else{
				if(m_hEnemy != NULL){
					if(( pev->origin - m_hEnemy->pev->origin).Length() <= 120){
					m_hEnemy->TakeDamage( pev, pev, dmg1, DMG_SLASH );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
			}


		}
		break;

		case 3:
		{
			if(m_rpgms_skill1_learn == 18 && RANDOM_LONG(0,100) <= 20){//����һ��
			dmg2 *= 3;
			}

			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 100;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg2, gpGlobals->v_forward, &tr, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );


				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );


			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 90, dmg2, DMG_SLASH );
				if(pHurt){

					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

				}
				else if(m_hEnemy != NULL){
					if(( pev->origin - m_hEnemy->pev->origin).Length() <= 120){
					m_hEnemy->TakeDamage( pev, pev, dmg2, DMG_SLASH );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
			}


		}
		break;

		case 5:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 80;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, 15, gpGlobals->v_forward, &tr, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );

				if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) ){
				pEntity->pev->velocity = (pEntity->pev->origin - pev->origin).Normalize() * 200;
				}

				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 80, 15, DMG_SLASH );
				if(pHurt){
					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) ){
					pHurt->pev->velocity = (pHurt->pev->origin - pev->origin).Normalize() * 200;
					}
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}


		}
		break;

		case 6:
		{
				CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
				if ( pEntity )//��������
				{
					pev->body = 2;
					CBaseMonster *pEnemyMonster = pEntity->MyMonsterPointer();
					pEnemyMonster->SetBodygroup( 2, 2 );
					pev->velocity = g_vecZero;
				}
		}
		break;

		case 7:
		{
			m_SkillTime = gpGlobals->time + 50;
			m_dyingtime = -80;
			m_canbarnacle_mode = 0;
			m_freeze_def = 2;//����ο���LV2
			pev->body = 1;
			EMIT_SOUND(ENT(pev), CHAN_AUTO, "giant/giant_ready.wav", 1, 0.5);
		}
		break;

		case 8:
		{
			pev->team = 666;//����Ч��
			::RadiusDamage_limit( pev->origin, pev, pev, 120, 1500, 999, DMG_SONIC | DMG_NEVERGIB);
			FX_Explosion( Center(), 120);
			EMIT_SOUND_DYN( ENT(pev), CHAN_AUTO, "giant/giant_song.wav", 1, 0.4, 0, 100);
			//EMIT_SOUND(ENT(pev), CHAN_AUTO, "giant/giant_song.wav", 1, 0.5);
		}
		break;

		case 9:
		{
			m_freeze_def = 0;
			pev->body = 0;
			pev->team = 0;
			ClearSchedule();
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

void CGiant :: RunAI( void )
{
	CBaseMonster :: RunAI();
	if(pev->sequence == LookupActivity ( ACT_RUN ) || pev->sequence == LookupActivity ( ACT_RUN_SCARED )){
    m_flGroundSpeed = 250;
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
	if(m_rpgms_level >= 45 && m_rpgms_skill2_learn == 0){
	pev->max_health = 1200;
	pev->health = pev->max_health;
	m_rpgms_skill1_learn = 18;//����һ��
	m_rpgms_skill2_learn = 43;//�����ɳ�
	m_rpgms_skill3_learn = 72;//�񵲷���
	m_rpgms_skill4_learn = 73;//����֮��
	m_rpgms_skill5_learn = 24;//Ӳ��
	}

	if(pev->armortype == 1){
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
}

//=========================================================
// Spawn
//=========================================================
void CGiant :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/giant.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= 600;
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();

	m_follow_mode = 1;

	SetUse( &CGiant::FollowerUse2 );

	m_ignoredamage = 1;
	m_lovehate = 40;
	pev->body = 0;

	m_selfmode = TRUE;
	m_longming = 1;//����

	m_rpgms_actor = 3;
	m_rpgms_level = 8;
	m_rpgms_exp = 0;
	m_rpgms_type = 1;

	m_new_ally_type = TRUE;
	pev->netname = MAKE_STRING( "Giant" );

	m_canbarnacle_mode = 1;

	m_rpgms_skill1_learn = 0;//����һ��
	m_rpgms_skill2_learn = 0;//�����ɳ�
	m_rpgms_skill3_learn = 0;//�񵲷���
	m_rpgms_skill4_learn = 0;//����֮��
	m_rpgms_skill5_learn = 0;//Ӳ��24

	pev->takedamage = DAMAGE_YES;
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGiant :: Precache()
{
	PRECACHE_MODEL("models/giant.mdl");
	
	PRECACHE_SOUND("giant/die.wav");
	PRECACHE_SOUND("giant/pain.wav");
	PRECACHE_SOUND("giant/giant_ready.wav");
	PRECACHE_SOUND("giant/giant_song.wav");
}	

int CGiant::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();
	return iIgnore;
}