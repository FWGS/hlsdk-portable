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
#include	"soundent.h"

extern DLL_GLOBAL int		g_iSkillLevel;

class ZDeadEye : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );

	void Killed( entvars_t *pevAttacker, int iGib );

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	int IRelationship( CBaseEntity *pTarget );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	Vector m_teleportorigin;

	void RunAI( void );
};

LINK_ENTITY_TO_CLASS( monster_zdeadeye, ZDeadEye );

TYPEDESCRIPTION	ZDeadEye::m_SaveData[] = 
{
	DEFINE_FIELD( ZDeadEye, m_teleportorigin, FIELD_POSITION_VECTOR ),
};

IMPLEMENT_SAVERESTORE( ZDeadEye, CBaseMonster );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	ZDeadEye :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

void ZDeadEye :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->deadflag == DEAD_NO){

		if(pev->movetype == MOVETYPE_FLY){

			if( !(pev->flags & FL_FLY) ){
				pev->flags	 |= FL_FLY;
				m_teleportorigin = pev->origin;
				if(pev->frags == 0){
				pev->velocity.x = 30;
				}
				else if(pev->frags == 1){
				pev->velocity.y = 30;
				}
				else if(pev->frags == 2){
				pev->velocity.z = 30;
				}
			}

			if(m_freezetime == 0){

				if(pev->frags == 0){//x浮动
					if (m_teleportorigin.x - pev->origin.x > 60){
						if(pev->velocity.x < 30){
						pev->velocity.x += 10;
						}
					}
					if (m_teleportorigin.x - pev->origin.x < -60){
						if(pev->velocity.x > -30){
						pev->velocity.x -= 10;
						}
					}
				}
				else if(pev->frags == 1){//y浮动
					if (m_teleportorigin.y - pev->origin.y > 60){
						if(pev->velocity.y < 30){
						pev->velocity.y += 10;
						}
					}
					if (m_teleportorigin.y - pev->origin.y < -60){
						if(pev->velocity.y > -30){
						pev->velocity.y -= 10;
						}
					}
				}
				else if(pev->frags == 2){//z浮动
					if (m_teleportorigin.z - pev->origin.z > 60){
						if(pev->velocity.z < 30){
						pev->velocity.z += 10;
						}
					}
					if (m_teleportorigin.z - pev->origin.z < -60){
						if(pev->velocity.z > -30){
						pev->velocity.z -= 10;
						}
					}
				}
			}
			else{
				pev->effects = 0;
			}
		}

		if(m_freezetime == 0){
			if(m_singdelay_use == 3){
				if(FBitSet(pev->effects, EF_LIGHT)){
				ClearBits(pev->effects, EF_LIGHT);
				}
				if(!FBitSet(pev->effects, EF_DIMLIGHT)){
				SetBits(pev->effects, EF_DIMLIGHT);
				}
			}
			else{
				if(!FBitSet(pev->effects, EF_LIGHT) && m_cAmmoLoaded == 0){
				SetBits(pev->effects, EF_LIGHT);
				}
			}
		}

		if(m_hEnemy != NULL && m_cAmmoLoaded == 0 && m_cleardally_enemy_long > 30){
			m_HackedGunPos = pev->origin;

			if ( HasConditions( bits_COND_SEE_ENEMY ) ){
				CBaseEntity *pEnemy = m_hEnemy;
				if ( pEnemy )
				{//精准射击の加持
				m_vecEnemyLKP = pEnemy->pev->origin;
				}
			}

			Vector vecShootDir = ShootAtEnemy( m_HackedGunPos );

			FireBullets(1, m_HackedGunPos, vecShootDir, g_vecZero, 16384, BULLET_IONTURRET,0,623);

			FireBeam(m_HackedGunPos, vecShootDir, 23, 100, pev);

			EMIT_SOUND_DYN( ENT(pev), CHAN_STREAM, "doma/doma_fire2.wav", 1, 0.6, 0, 100);

			m_cAmmoLoaded = 60;

			if(FBitSet(pev->effects, EF_LIGHT)){
			ClearBits(pev->effects, EF_LIGHT);
			}

			m_singdelay_use = m_singdelay_max;
		}
		else if(m_cAmmoLoaded > 0){
			m_cAmmoLoaded--;
		}
	}

}

void ZDeadEye :: SetYawSpeed ( void )
{
	pev->yaw_speed = 360;
}

void ZDeadEye::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20))
	{
	UTIL_Sparks(ptr->vecEndPos);
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

int ZDeadEye :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( (bitsDamageType & DMG_GENERIC)){
	flDamage *= 0.5;
	}
	if ( (bitsDamageType & DMG_ENERGYBEAM)){
	flDamage *= 0.8;
	}
	if ( (bitsDamageType & DMG_DARK)){//弱点
	flDamage *= 1.25;
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void ZDeadEye::Killed( entvars_t *pevAttacker, int iGib )
{
	if(pev->takedamage){
	pev->effects = 0;
	FX_Explosion( Center(), 137 );
	pev->takedamage = DAMAGE_NO;
	pev->solid = SOLID_NOT;
	FadeMonster();
	}

	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

int ZDeadEye::IRelationship ( CBaseEntity *pTarget )
{
	if(pTarget->Classify() == CLASS_PLAYER_BIOWEAPON 
	|| pTarget->Classify() == CLASS_ALIEN_BIOWEAPON){
	return R_NO;//无视干扰
	}

	return CBaseMonster :: IRelationship( pTarget );
}

//=========================================================
// Spawn
//=========================================================
void ZDeadEye :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/zdeadeye.mdl");
	UTIL_SetSize( pev, Vector( -32, -32, -32 ), Vector( 32, 32, 32 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	m_bloodColor		= DONT_BLEED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 3600;
	}
	else{
	pev->health			= 3000;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= -1;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();

	m_aimenemy_mod = 6;
	m_MoveFail_SimpleRoad = TRUE;
	m_MoveFail_FuckRoad = TRUE;
	m_ignoredamage  = 1;

	m_killed_exp = 1200;
	m_rpgms_level = 90;
	pev->gravity = 1.6;

	m_attack_dist = 2560;
	pev->netname = MAKE_STRING( "Z.Dead.Eye" );

	m_enemyget_mode = 2;//无视掉无敌の慧眼
	m_EyeMod = 2;//目力模式
	m_longming = 1;//龙鸣
	m_selfmode = 1;//自私模式

	SetBits(pev->flags, FL_SWIM);//水+飞，穿透视线

	SetBits(pev->effects, EF_DIMLIGHT);

	m_singdelay_max = 3;//反应略慢
	m_singdelay_use = m_singdelay_max;
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void ZDeadEye :: Precache()
{
	PRECACHE_MODEL("models/zdeadeye.mdl");
	PRECACHE_SOUND("doma/doma_fire1.wav");
	PRECACHE_SOUND("doma/doma_fire2.wav");
}
