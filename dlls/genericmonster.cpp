/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
// Generic Monster - purely for scripted sequence work.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"

// For holograms, make them not solid so the player can walk through them
#define SF_GENERICMONSTER_NOTSOLID 4

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CGenericMonster : public CBaseMonster
{
public:
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
	CGenericMonster() : m_bSetPVSDefault(FALSE), m_iLightValue(0) {}
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int Classify() override;
	void HandleAnimEvent( MonsterEvent_t* event ) override;
	int ISoundMask() override;
	void SetActivity( Activity activity ) override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	void KeyValue( KeyValueData* data ) override;
	void EXPORT UseTarget( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );
	int ObjectCaps() override;
	static TYPEDESCRIPTION m_SaveData[];
	BOOL m_bSetPVSDefault;
	BOOL m_nopvs;
	int m_iLightValue;
};

LINK_ENTITY_TO_CLASS( monster_generic, CGenericMonster );

TYPEDESCRIPTION CGenericMonster::m_SaveData[] =
{
	DEFINE_FIELD( CGenericMonster, m_bSetPVSDefault, FIELD_BOOLEAN ),
	DEFINE_FIELD( CGenericMonster, m_iLightValue, FIELD_INTEGER ),
	DEFINE_FIELD( CGenericMonster, m_nopvs, FIELD_BOOLEAN )
};

int CGenericMonster::Save( CSave& save )
{
	if ( !CBaseMonster::Save( save ) )
		return 0;
	return save.WriteFields( "CGenericMonster", this, m_SaveData, ARRAYSIZE( m_SaveData ) );
}

int CGenericMonster::Restore( CRestore& restore )
{
	int restored = CBaseMonster::Restore( restore );
	if ( restored )
	{
		restored = restore.ReadFields( "CGenericMonster", this, m_SaveData, ARRAYSIZE( m_SaveData ) );
		pev->iuser2 = m_nopvs;
		if ( m_iLightValue )
			pev->iuser4 = m_iLightValue;
	}
	return restored;
}

int CGenericMonster::Classify()
{
	return CLASS_PLAYER_ALLY;
}

int CGenericMonster::ObjectCaps()
{
	return FCAP_IMPULSE_USE | FCAP_ACROSS_TRANSITION;
}

int CGenericMonster::ISoundMask()
{
	return 0;
}

void CGenericMonster::SetYawSpeed()
{
	pev->yaw_speed = 90;
}

void CGenericMonster::HandleAnimEvent( MonsterEvent_t* event )
{
	CBaseMonster::HandleAnimEvent( event );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGenericMonster::Precache()
{
	PRECACHE_MODEL( (char*)STRING( pev->model ) );
}

//=========================================================
// Spawn
//=========================================================
void CGenericMonster::Spawn()
{
	Precache();
	SET_MODEL( edict(), STRING( pev->model ) );

/*
	if ( FStrEq( STRING(pev->model), "models/player.mdl" ) )
		UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
*/

	if ( FStrEq( STRING( pev->model ), "models/player.mdl" ) || FStrEq( STRING( pev->model ), "models/holo.mdl" ) )
		UTIL_SetSize( pev, VEC_HULL_MIN, VEC_HULL_MAX );
	else
		UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
	pev->iuser1 = 0;
	pev->flags |= FL_IMMUNE_LAVA;
	pev->solid = SOLID_SLIDEBOX;
	if ( !m_dontFall )
		pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	if ( pev->health == 0 )
		pev->health = 8;
	m_flFieldOfView = 0.5f; // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	MonsterInit();
	SetUse( &CGenericMonster::UseTarget );
	if ( pev->spawnflags & SF_GENERICMONSTER_NOTSOLID )
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
	}
	if ( m_iLightValue )
	{
		pev->renderfx = 21;
		pev->iuser4 = m_iLightValue;
	}
	if ( pev->rendermode == kRenderNormal && pev->renderamt == 0 )
	{
		pev->renderamt = 255;
		pev->rendermode = kRenderTransAlpha;
	}
	if ( m_invulnerable )
		pev->takedamage = DAMAGE_NO;
}

void CGenericMonster::KeyValue( KeyValueData* data )
{
	if ( !m_bSetPVSDefault )
	{
		pev->iuser2 = 0;
		m_nopvs = FALSE;
		m_bSetPVSDefault = TRUE;
	}
	if ( FStrEq( data->szKeyName, "nopvs" ) )
	{
		pev->iuser2 = atoi( data->szValue );
		m_nopvs = pev->iuser2 != 0;
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "customlightmultiplier" ) )
	{
		m_iLightValue = (int)(atof( data->szValue ) * 10);
		data->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( data );
}

void CGenericMonster::UseTarget( CBaseEntity* activator, CBaseEntity*, USE_TYPE useType, float value )
{
	if ( m_useTarget )
		FireTargets( STRING( m_useTarget ), activator, this, useType, value );
}

void CGenericMonster::SetActivity( Activity activity )
{
	GET_MODEL_PTR( edict() );
	int animation = 0;
	if ( (activity >= ACT_DIESIMPLE && activity <= ACT_DIEVIOLENT)
		|| (activity >= ACT_DIE_HEADSHOT && activity <= ACT_DIE_BACKSHOT) )
		animation = m_deathAnim;
	else if ( activity == ACT_IDLE )
		animation = m_idleAnim;
	int sequence = animation ? LookupSequence( STRING( animation ) ) : LookupActivity( activity );
	m_Activity = activity;
	if ( sequence < 0 )
	{
		ALERT( at_console, "%s has no sequence for act:%d\n", STRING( pev->classname ), activity );
		pev->sequence = 0;
		return;
	}
	if ( sequence != pev->sequence || m_fSequenceLoops )
		pev->frame = 0;
	pev->sequence = sequence;
	ResetSequenceInfo();
	SetYawSpeed();
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
