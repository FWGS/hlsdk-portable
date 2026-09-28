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

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

Task_t	tlStone_wakeup[] =
{
	{ TASK_STOP_MOVING,					0				  },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_STAND },
};

Schedule_t slStone_wakeup[] = 
{
	{
		tlStone_wakeup,
		ARRAYSIZE ( tlStone_wakeup ),
		0,
		0,
		"Stone Wake Up"
	}
};


class CStoneDevil : public CBaseMonster
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

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void DeathSound( void );

	BOOL	m_fGunDrawn;

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	void RunAI( void );

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_stone_devil, CStoneDevil );
LINK_ENTITY_TO_CLASS( monster_stone_devil_h, CStoneDevil );
LINK_ENTITY_TO_CLASS( monster_stone_devil_s, CStoneDevil );

TYPEDESCRIPTION	CStoneDevil::m_SaveData[] = 
{
	DEFINE_FIELD( CStoneDevil, m_fGunDrawn, FIELD_BOOLEAN ),
};
IMPLEMENT_SAVERESTORE( CStoneDevil, CBaseMonster );

DEFINE_CUSTOM_SCHEDULES( CStoneDevil )
{
	slStone_wakeup,
};

IMPLEMENT_CUSTOM_SCHEDULES( CStoneDevil, CBaseMonster );
//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CStoneDevil :: Classify ( void )
{
	if(pev->team == 1){
		return	CLASS_ALIEN_MILITARY;
	}
	else{
		return	CLASS_HUMAN_ASS;
	}
}

void CStoneDevil :: DeathSound ( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "monster/stone_die.wav", 1, 0.6, 0, 100);
}

void CStoneDevil :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->sequence == LookupActivity ( ACT_WALK )){
	m_flGroundSpeed = 180;
	}

	if(m_cAmmoLoaded < 60){
	m_cAmmoLoaded++;
	}

	if(pev->team == 1 && pev->deadflag == DEAD_NO && pev->movetype == MOVETYPE_STEP){
		if(!WALK_MOVE ( ENT(pev), 0, 0, WALKMOVE_NORMAL ) ){//石头卡住了？
		pev->armorvalue++;
		}
		else{
		pev->armorvalue--;
		}
		if(pev->armorvalue >= 60){
		Killed( pev, GIB_NEVER );
		}
	}

}


//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CStoneDevil :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 120 && flDot >= 0.5 && m_hEnemy != NULL)
	{
		return TRUE;
	}

	return FALSE;
}


BOOL CStoneDevil :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if (flDist <= 2048 && m_hEnemy != NULL && m_cAmmoLoaded >= 60){
	m_facing_fucking_mode = 1;
	return TRUE;
	}
	else{
	m_facing_fucking_mode = 0;
	}

	return FALSE;
}

void CStoneDevil::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20))
	{
	pev->dmgtime = gpGlobals->time;
	UTIL_Sparks(ptr->vecEndPos);
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

Schedule_t* CStoneDevil :: GetScheduleOfType ( int Type )
{
	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slStone_wakeup;
		}
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CStoneDevil :: GetSchedule ( void )
{
	if (!m_fGunDrawn && (m_hEnemy != NULL || pev->health <= 0 || m_allydeadcheck == 1) ){
		return GetScheduleOfType( SCHED_ARM_WEAPON );
	}

	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CStoneDevil :: SetYawSpeed ( void )
{
	if(m_fGunDrawn){
	pev->yaw_speed = 180;
	}
	else{
	pev->yaw_speed = 0;
	}
}

int CStoneDevil :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if(FBitSet( pev->flags, FL_NOTARGET )){//防御模式
	flDamage *= 0.1;
	}

	if ( (bitsDamageType & DMG_CRUSH)){
	return 0;
	}

	if ( (bitsDamageType & DMG_ENERGYBEAM)
	|| (bitsDamageType & DMG_BULLET)){
		flDamage *= 0.8;
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CStoneDevil::Killed( entvars_t *pevAttacker, int iGib )
{
	if(m_cAmmoLoaded == 100){
	FX_Trail(pev->origin, entindex(), PROJ_REMOVE);
	}
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CStoneDevil :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg1;
	dmg1 = 80;//大地之槌

	switch( pEvent->event )
	{
		case 1:
		{
			pev->flags |= FL_NOTARGET;
			UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
		}
		break;

		case 2:
		{
			UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 128 ) );
			pev->flags &= ~FL_NOTARGET;
		}
		break;

		case 3:
		{
			UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 192 ) );
		}
		break;

		case 4:
			{
				UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 128 ) );
				m_crouchmode        = 0;
				m_fGunDrawn         = TRUE;
				m_flDistLook		= 4096;
				m_flFieldOfView		= 0.5;
				pev->frags			= 0;
				pev->flags		   &= ~FL_NOTARGET;
				m_facing_fucking_mode = 1;
				if(pev->health < 1){
				pev->health = 1;
				}
			}
			break;

		case 5:
		{
				//Vector vecStart, angleGun;
				//GetAttachment( 0, vecStart, angleGun );
				FX_Trail(pev->origin, entindex(), 137);
				m_cAmmoLoaded = 100;
		}
		break;

		case 6:
		{
			FX_Trail(pev->origin, entindex(), PROJ_REMOVE);

			if(m_hEnemy != NULL && m_cAmmoLoaded >= 60){
			Vector org,vecdir;
			GetAttachment( 0, org,vecdir);
			m_HackedGunPos = org;
			Vector vecShootDir = ShootAtEnemy( m_HackedGunPos );

			Vector angDir = UTIL_VecToAngles( vecShootDir );
			SetBlending( 0, angDir.x );

			UTIL_Sparks( m_HackedGunPos );

			FireBullets(1, m_HackedGunPos, vecShootDir, g_vecZero, 16384, BULLET_IONTURRET,0);

			FireBeam(m_HackedGunPos, vecShootDir, 20, 100, pev);

			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/gauss2.wav", 1, ATTN_NORM, 0, 100);
			}

			m_cAmmoLoaded = 0;
		}
		break;

		case 7:
		{
			UTIL_MakeVectors(pev->angles);
			Vector hit_org = pev->origin + gpGlobals->v_forward * 120;
			FX_Explosion( hit_org, EXPLOSION_TORCH);
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "tyant_boss/blast.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			if(pev->team == 1){//Bug Fix 3.0 石头怪不误伤友军
			::RadiusDamage_limit( hit_org, pev, pev, dmg1, 160, CLASS_ALIEN_MILITARY, DMG_CRUSH);
			}
			else{
			::RadiusDamage_limit( hit_org, pev, pev, dmg1, 160, CLASS_HUMAN_ASS, DMG_CRUSH);
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
void CStoneDevil :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/stone_devil.mdl");
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= DONT_BLEED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 600;
	}
	else{
	pev->health			= 480;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();

	if( FClassnameIs(pev, "monster_stone_devil_h") || FClassnameIs(pev, "monster_stone_devil_s") ){
		m_crouchmode        = 0;
		m_fGunDrawn         = TRUE;
		m_flFieldOfView		= -1;
		UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 128 ) );
		m_cAmmoLoaded		= 45;//快速蓄力

		if(FClassnameIs(pev, "monster_stone_devil_h")){
		pev->team = 1;
		}

		pev->classname = MAKE_STRING("monster_stone_devil");
	}
	else if(pev->frags == 0){
	m_crouchmode        = 1;
	m_fGunDrawn         = FALSE;
	m_flFieldOfView		= -1;
	m_flDistLook        = 1024;
	pev->flags			|= FL_NOTARGET;
	}

	m_aimenemy_mod = 6;
	m_MoveFail_SimpleRoad = TRUE;
	m_MoveFail_FuckRoad = TRUE;
	m_ignoredamage  = 1;
	m_headdef		= 2;

	m_killed_exp = 200;
	m_rpgms_level = 60;
	pev->gravity = 1.6;

	m_attack_dist = 384;
	pev->netname = MAKE_STRING( "Stone.Devil" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CStoneDevil :: Precache()
{
	PRECACHE_MODEL("models/stone_devil.mdl");

	PRECACHE_SOUND ("tyant_boss/blast.wav");
	PRECACHE_SOUND ("monster/stone_die.wav");
}	
