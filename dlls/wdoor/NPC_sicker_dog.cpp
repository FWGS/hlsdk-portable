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

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CSickerDog : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void Killed( entvars_t *pevAttacker, int iGib );

	void AttackSound( void );
	void DeathSound( void );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

};

LINK_ENTITY_TO_CLASS( monster_sicker_dog, CSickerDog );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CSickerDog :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

void CSickerDog::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CSickerDog :: SetYawSpeed ( void )
{
	pev->yaw_speed = 150;
}

int CSickerDog :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	pev->spawnflags = 0;
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CSickerDog::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

void CSickerDog :: DeathSound ( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "sicker_dog/die.wav", 1, 0.6, 0, 100);
}

void CSickerDog :: AttackSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "sicker_dog/attack.wav", 1, 0.6, 0, 100);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CSickerDog :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;
	dmg = 30;

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

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH | DMG_NEVERGIB ); 
			ApplyMultiDamage( pev, pev );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, dmg, DMG_SLASH | DMG_NEVERGIB );
			}

			AttackSound();
			
		}
		break;
		
		case 2:
		{
		UTIL_SetSize ( pev, Vector ( pev->mins.x, pev->mins.y, pev->mins.z ), Vector ( pev->maxs.x, pev->maxs.y, pev->mins.z + 4 ) );
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
void CSickerDog :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/sicker_dog.mdl");
	UTIL_SetSize(pev, Vector ( -16, -16, 0 ), Vector ( 16, 16, 64 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 120;
	}
	else{
	pev->health			= 90;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	
	MonsterInit();

	m_ignoredamage = 1;

	m_forcefuckdoor  = TRUE;
	m_selfmode = TRUE;
	m_MoveFail_SimpleRoad = TRUE;

	m_killed_exp = 60;
	m_rpgms_level = 30;
	pev->netname = MAKE_STRING( "Sicker.Dog" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSickerDog :: Precache()
{
	PRECACHE_MODEL("models/sicker_dog.mdl");

	PRECACHE_SOUND("sicker_dog/die.wav");
	PRECACHE_SOUND("sicker_dog/attack.wav");
}	
