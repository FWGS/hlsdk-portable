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

class CEmpty_zombie : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void Killed( entvars_t *pevAttacker, int iGib );

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

};


LINK_ENTITY_TO_CLASS( monster_empty_zombie, CEmpty_zombie );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CEmpty_zombie :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

void CEmpty_zombie::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

Schedule_t* CEmpty_zombie :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CEmpty_zombie :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CEmpty_zombie :: SetYawSpeed ( void )
{
	pev->yaw_speed = 120;
}

int CEmpty_zombie :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CEmpty_zombie::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, iGib );

	if(m_LastHitGroup == 1 && GetBodygroup( 0 ) == 0 && m_gibed == 0){
	SetBodygroup( 0, 1 );
	Vector vecGunPos,vecGunAngles;
	GetAttachment( 0, vecGunPos, vecGunAngles );
	FX_Explosion( vecGunPos, 236 );
	SpawnBlood(vecGunPos, BloodColor(), 100);
	//FX_Trail(vecGunPos, entindex(), PROJ_GUTS );
	EMIT_SOUND(ENT(pev), CHAN_BODY, "newadd/zom_headburst.wav", 1, ATTN_NORM);	
	}
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CEmpty_zombie :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;
	dmg = 10;

	switch( pEvent->event )
	{
		case 1:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 75;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "zombie/claw_strike1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, dmg, DMG_SLASH );
				if(pHurt){
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "zombie/claw_strike1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}

		}
		break;

		case 2:
		{
			dmg *= 2.0;

			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 75;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "zombie/claw_strike1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, dmg, DMG_SLASH );
				if(pHurt){
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "zombie/claw_strike1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
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
void CEmpty_zombie :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/empty_zombie.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	
	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 50;
	}
	else{
	pev->health			= 40;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();

	m_killed_exp = 10;
	m_rpgms_level = 5;
	m_thinkspeed = 2;//ÂýËÙË¼¿¼£¬×îÈõ¤Î¹Ö
	m_attack_dist = 128;
	pev->netname = MAKE_STRING( "Empty.Zombie" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CEmpty_zombie :: Precache()
{
	PRECACHE_MODEL("models/empty_zombie.mdl");

	PRECACHE_SOUND( "zombie/claw_strike1.wav" );
}	
