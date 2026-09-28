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

class CDman : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void Killed( entvars_t *pevAttacker, int iGib );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void RunAI( void );

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};

LINK_ENTITY_TO_CLASS( monster_drop_man, CDman );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
void CDman :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->weapons != 0){
	return;
	}

	if(pev->frags == 0 && pev->deadflag == DEAD_NO){
		CBaseEntity *pEntity = NULL;
		while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 300 )) != NULL)
		{
				if (  pEntity != this && !FBitSet( pEntity->pev->flags, FL_NOTARGET ) &&
					(pEntity->pev->movetype == MOVETYPE_FLY 
					|| pEntity->pev->movetype == MOVETYPE_STEP
					|| pEntity->pev->movetype == MOVETYPE_WALK) ){
						SetActivity ( ACT_USE );

						::RadiusDamage3( Center(), pev, pev, 450, 600, CLASS_HUMAN_ASS, DMG_BLAST);
						FX_Trail( Center(), entindex(), 139 );

						pev->frags = 6;
						break;
				}
		}
	}
	else{
		pev->frags -= 1;
	}

	
}

int	CDman :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

void CDman::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CDman :: SetYawSpeed ( void )
{
	pev->yaw_speed = 360;
}

int CDman :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CDman::Killed( entvars_t *pevAttacker, int iGib )
{
	if(pev->effects != 0){
	pev->effects = 0;
	m_diefadeout = 1;
	}

	CBaseMonster::Killed( pevAttacker, iGib );
}



//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CDman :: HandleAnimEvent( MonsterEvent_t *pEvent )
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
void CDman :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/dman.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->health			= 400;

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();

	pev->effects		= EF_DIMLIGHT;
	pev->netname = MAKE_STRING( "DMan" );
	m_killed_exp = 10;
	m_rpgms_level = 10;
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CDman :: Precache()
{
	PRECACHE_MODEL("models/dman.mdl");
}	
