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

class CSlime : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void Killed( entvars_t *pevAttacker, int iGib );

	void RunAI( void );

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

};

LINK_ENTITY_TO_CLASS( monster_slime, CSlime );

void CSlime :: RunAI( void )
{
	if(pev->deadflag == DEAD_NO){

		if(pev->weapons == 1){//黑水模式，未发现敌人前隐身，不被突袭
			m_flDistLook = 768.0;
			m_singdelay_max = 0;
			m_singdelay_use = m_singdelay_max;
			pev->weapons = 2;
			pev->takedamage = DAMAGE_NO;
			pev->effects |= EF_NODRAW;
			m_longming = 1;
			m_selfmode = TRUE;
			pev->solid	= SOLID_NOT;
		}
		else if(pev->weapons >= 2){
			if(pev->weapons == 2 && m_hEnemy != NULL){
			pev->weapons = 3;
			pev->takedamage = DAMAGE_AIM;
			pev->effects &= ~EF_NODRAW;
			pev->solid	= SOLID_SLIDEBOX;
			}
			if ( pev->waterlevel >= 1){
			Killed( pev, GIB_ALWAYS );//碎尸!
			return;
			}
		}

		if(pev->sequence == LookupActivity ( ACT_WALK )){
		m_flGroundSpeed = 300;
		}
		if(m_hEnemy != NULL){
			if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) <= 128 )
			{
				if(( pev->origin - m_hEnemy->pev->origin).Length2D() <= 64){
				Killed( pev, GIB_NEVER );
				}
			}
		}
	}
	CBaseMonster :: RunAI();
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CSlime :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CSlime :: SetYawSpeed ( void )
{
	pev->yaw_speed = 150;
}

int CSlime :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CSlime::Killed( entvars_t *pevAttacker, int iGib )
{
	if(m_die == 0){
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->effects |= EF_NODRAW;
	FadeMonster();

	EMIT_SOUND(ENT(pev), CHAN_BODY, "newadd/slime_blast1.wav", 1, ATTN_NORM);		
	FX_Explosion(pev->origin, 239 );
	::RadiusDamage2(pev->origin, pev, pev, 80, 160, CLASS_HUMAN_ASS, DMG_BLOOD);
	m_die = 1;
	return;
	}

	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CSlime :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case 1:
		{

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
void CSlime :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/slime.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	
	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 20;
	}
	else{
	pev->health			= 10;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= -1;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();

	m_ignoreFail_MAX = 30;
	m_ignoreFail_OFF = 0;
	m_MoveFail_FuckRoad = TRUE;
	m_MoveFail_SimpleRoad = TRUE;
	m_killed_exp = 10;
	m_rpgms_level = 10;
	pev->netname = MAKE_STRING( "Slime" );

	m_singdelay_max = 1;//快速反应
	m_singdelay_use = m_singdelay_max;
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSlime :: Precache()
{
	PRECACHE_MODEL("models/slime.mdl");
	PRECACHE_SOUND("newadd/slime_blast1.wav" );
}	
