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
#include	"game.h"
#include	"player.h"
#include	"soundent.h"

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CCrasher : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	float m_flNextFlinch;
	float m_flNextPainTime;

	void Killed( entvars_t *pevAttacker, int iGib );

	void PainSound( void );
	void DeathSound( void );
	void RunAI( void );

	BOOL CheckMeleeAttack1 ( float flDot, float flDist );//¾Þ×¦
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );//¶¾Æø
	BOOL CheckRangeAttack1 ( float flDot, float flDist );//ÅÚµ¯


	BOOL FCanCheckAttacks ( void );
	Schedule_t* GetSchedule ( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	static const char *pAttackSounds[];
	static const char *pDeathSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void StartTask ( Task_t *pTask );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	Vector m_shotorg;
	int m_GasUse;
	float m_flNextGasTime;// we keep track of this, because if something hurts a squid, it will forget about its love of headcrabs for a while.
	float m_flNextRocketTime;// last time the bullsquid used the spit attack.
};

LINK_ENTITY_TO_CLASS( monster_crasher_boss, CCrasher );

TYPEDESCRIPTION	CCrasher::m_SaveData[] = 
{
	DEFINE_FIELD( CCrasher, m_shotorg, FIELD_VECTOR ),
	DEFINE_FIELD( CCrasher, m_GasUse, FIELD_INTEGER ),
	DEFINE_FIELD( CCrasher, m_flNextGasTime, FIELD_TIME ),
	DEFINE_FIELD( CCrasher, m_flNextRocketTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CCrasher, CBaseMonster );

const char *CCrasher::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CCrasher::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CCrasher::pDeathSounds[] = 
{
	"gonome/gonome_death2.wav",
	"gonome/gonome_death3.wav",
};

const char *CCrasher::pPainSounds[] = 
{
	"gonome/gonome_pain1.wav",
	"gonome/gonome_pain2.wav",
	"gonome/gonome_pain3.wav",
};


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CCrasher :: Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}
//=========================================================
// RunAI
//=========================================================
void CCrasher :: RunAI( void )
{
	CBaseMonster :: RunAI();
	if(pev->sequence == LookupActivity ( ACT_WALK )
	|| pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
	m_flGroundSpeed = 100;
	}

	int dmg;
	dmg = 5;

	if(m_GasUse > 0){
		m_GasUse--;
		::RadiusDamage( Center(), pev, pev, dmg, 350, CLASS_NONE, DMG_NERVEGAS);
		if(m_GasUse == 1){
		STOP_SOUND( edict(), CHAN_STREAM, "weapons/gas_explode.wav" );
		FX_Trail( pev->origin, entindex(), PROJ_REMOVE );
		}
	}
	
}

void CCrasher::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	//¿ø¼×
	if (ptr->iHitgroup == 11)
	{
		flDamage *= 0.5;
		if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20))
		{
		pev->dmgtime = gpGlobals->time;
		UTIL_WhiteSparks( ptr->vecEndPos, ptr->vecPlaneNormal, 9, 5, 5, 100 );//puntos
		UTIL_Sparks( ptr->vecEndPos );
		}
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	if (ptr->iHitgroup == 10)
	{
		if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20))
		{
		pev->dmgtime = gpGlobals->time;
		UTIL_WhiteSparks( ptr->vecEndPos, ptr->vecPlaneNormal, 9, 5, 5, 100 );//puntos
		UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT(0.5,1.5) );
		}
		flDamage *= 0.1;
		return;
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CCrasher :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_RANGE_ATTACK1:	
		ys = 60;	
		break;
	default:
		ys = 180;
		break;
	}

	pev->yaw_speed = ys;
}

BOOL CCrasher :: FCanCheckAttacks ( void )
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


BOOL CCrasher :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if (flDist <= 150 && m_hEnemy != NULL && flDot >= 0.5)
	{
			if(pev->sequence == LookupActivity ( ACT_WALK )){
			pev->sequence = LookupActivity ( ACT_WALK_SCARED );
			ResetSequenceInfo( );
			pev->frame =  0;
			}
	}
	else if (flDist >= 200 || flDot < 0)
	{
			if(pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
			pev->sequence = LookupActivity ( ACT_WALK );
			ResetSequenceInfo( );
			pev->frame = 0;
			}
	}

	return FALSE;
}

BOOL CCrasher :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	if(flDist <= 400 && m_flNextGasTime <= gpGlobals->time 
	&& m_hEnemy != NULL && !HasConditions(bits_COND_CAN_RANGE_ATTACK1) ){
		m_facing_fucking_mode = 1;
		return TRUE;
	}

	m_facing_fucking_mode = 0;
	return FALSE;
}



Schedule_t *CCrasher:: GetSchedule ( void )
{
	return CBaseMonster :: GetSchedule();
}


BOOL CCrasher :: CheckRangeAttack1 ( float flDot, float flDist )
{
	float limit_hp;
	limit_hp = 0.7;

	if ( flDist >= 250 && flDist <= 2000 && flDot >= 0.5 && m_hEnemy != NULL && m_flNextRocketTime <= gpGlobals->time
	&& m_cleardally_enemy > 0 && pev->health <= pev->max_health * limit_hp)
	{
		if ( m_hEnemy != NULL )
		{
			if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 200 )
			{
				return FALSE;
			}
		}

		return TRUE;
	}

	return FALSE;
}

int CCrasher :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if(bitsDamageType == DMG_NERVEGAS){
		return 0;
	}

	if( bitsDamageType & DMG_BLAST ){
		flDamage *= 1.25;
	}

	m_alert = 100;

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CCrasher::Killed( entvars_t *pevAttacker, int iGib )
{
	if(m_GasUse > 1){
		m_GasUse = 0;
		STOP_SOUND( edict(), CHAN_STREAM, "weapons/gas_explode.wav" );
		FX_Trail( pev->origin, entindex(), PROJ_REMOVE );
	}

	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

void CCrasher :: PainSound( void )
{
	if ( m_flNextPainTime > gpGlobals->time )
	{
		return;
	}

	m_flNextPainTime = gpGlobals->time + 0.75;

	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CCrasher :: DeathSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pDeathSounds[ RANDOM_LONG(0,ARRAYSIZE(pDeathSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CCrasher :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK2:
		{
			m_IdealActivity = ACT_RANGE_ATTACK2;
			break;
		}
	default:
		{
			CBaseMonster :: StartTask( pTask );
		}
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
// LeapTouch - this is the headcrab's touch function when it
// is in the air
//=========================================================


void CCrasher :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;
	float time;
	dmg = 70;
	time = 8.0;

	switch( pEvent->event )
	{
		case 1:
		{

			if(m_hEnemy != NULL){
					TraceResult tr;
					UTIL_MakeVectors(pev->angles);
					
					Vector vecSrc	= BodyTarget_c( pev->origin );
					Vector vecEnd	= m_hEnemy->Center();
					UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
					CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

					if ( tr.flFraction < 1.0 ){
						if(( vecSrc - tr.vecEndPos).Length() <= 110){
						int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
						int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
						FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

						ClearMultiDamage( );
						pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH); 

						if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) )
						{
						pEntity->pev->velocity = (pEntity->pev->origin - pev->origin).Normalize() * 300;
						}

						ApplyMultiDamage( pev, pev );
						EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
						}
					}
			}


		}
		break;

		case 2:
		{
			if(m_GasUse == 0){
			m_GasUse = 80;
			m_flNextGasTime = gpGlobals->time + 15.0;
			FX_Trail( Center(), entindex(), PROJ_NERVEGREN_DETONATE);
			EMIT_SOUND_DYN ( ENT(pev), CHAN_STREAM, "weapons/gas_explode.wav", 1.0, ATTN_NORM, 0, 100 );
			}
		}
		break;

		case 3:
		{
			Vector vecGunPos, vecAng;
			GetAttachment( 1, vecGunPos, vecAng );
			UTIL_MakeVectors(pev->angles);

			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc = pev->origin;
			vecSrc.z = vecGunPos.z;
			UTIL_TraceLine( vecSrc, vecGunPos, dont_ignore_monsters, ENT( pev ), &tr );
			if ( tr.flFraction < 1.0 ){
			//Fa?
			ClearSchedule();
			SetActivity( ACT_IDLE );
			pev->velocity.x += RANDOM_LONG(-250,250);
			pev->velocity.y += RANDOM_LONG(-250,250);
			pev->velocity.z += 50;
			m_flNextRocketTime = gpGlobals->time + 2.0;
			}
			m_shotorg = ShootAtEnemy( vecGunPos );
		}
		break;

		case 4:
		{
			if(m_flNextRocketTime <= gpGlobals->time){
			Vector vecGunPos, vecAng;
			GetAttachment( 1, vecGunPos, vecAng );
			UTIL_MakeVectors(pev->angles);

			FireBullets( 1, vecGunPos, m_shotorg * 0.75 + gpGlobals->v_forward * 0.25, Vector(0.02,0.02,0.02), 2048, 811, 0 );
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "tank/tank_fire.wav", 1, 0.4);
			pev->effects = pev->effects | EF_MUZZLEFLASH;

			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 512, 0.3 );

			m_flNextRocketTime = gpGlobals->time + RANDOM_FLOAT( 3.0, time );
			}
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
void CCrasher :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/crasher_boss.mdl");
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 80 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 3000;
	}
	else{
	pev->health			= 2400;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.3;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	pev->body			= 0;
	pev->gravity		= 1.6;

	m_flNextGasTime = gpGlobals->time + 6.0;
	m_flNextRocketTime = gpGlobals->time + 6.0;
	m_GasUse = 0;

	//m_headdef = 1;
	//m_chase_mode = 3;
	//m_chase_failed_max = 2;
	m_ignoredamage = 1;
	m_aimenemy_mod = 5;
//	m_walkaround = TRUE;
//	m_walkaroundFail = TRUE;
	m_MoveFail_SimpleRoad = TRUE;

	MonsterInit();
	m_killed_exp = 900;
	m_rpgms_level = 55;
	m_is_the_boss = TRUE;
	pev->netname = MAKE_STRING( "Crasher" );

	m_MoveStuckCheck = 1;
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCrasher :: Precache()
{
	int i;

	PRECACHE_MODEL("models/crasher_boss.mdl");
	PRECACHE_SOUND ("tank/tank_fire.wav");
	PRECACHE_SOUND("bullchicken/bc_spithit2.wav" );

	PRECACHE_SOUND("weapons/gas_explode.wav" );

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pDeathSounds ); i++ )
		PRECACHE_SOUND((char *)pDeathSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);
}	
