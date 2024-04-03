/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//=========================================================
// Monster Maker - this is an entity that creates monsters
// in the game.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "saverestore.h"
#include "monstermaker.h"
#include "effects.h"

//=========================================================
// MonsterMaker - this ent creates monsters during the game.
//=========================================================

LINK_ENTITY_TO_CLASS( monstermaker, CMonsterMaker )
LINK_ENTITY_TO_CLASS( env_warpball, CMonsterMaker )

TYPEDESCRIPTION	CMonsterMaker::m_SaveData[] =
{
	DEFINE_FIELD( CMonsterMaker, m_iszMonsterClassname, FIELD_STRING ),
	DEFINE_FIELD( CMonsterMaker, m_cNumMonsters, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_cLiveChildren, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_flGround, FIELD_FLOAT ),
	DEFINE_FIELD( CMonsterMaker, m_iMaxLiveChildren, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_fActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMonsterMaker, m_fFadeChildren, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMonsterMaker, m_fIsWarpBall, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMonsterMaker, m_cTotalMonstersCount, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CMonsterMaker, CBaseMonster )

void CMonsterMaker::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "monstercount" ) )
	{
		m_cNumMonsters = atoi( pkvd->szValue );
		m_cTotalMonstersCount = m_cNumMonsters;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "m_imaxlivechildren") || FStrEq(pkvd->szKeyName, "maxlivechildren"))
	{
		m_iMaxLiveChildren = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "monstertype" ) )
	{
		m_iszMonsterClassname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "warptarget") || FStrEq(pkvd->szKeyName, "makertarget") || FStrEq(pkvd->szKeyName, "warp_target") )
	{
		m_iszWarpTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "monsterspawnflags") ) 	// monsterspawnflags
	{
		m_iChildrenSpawnflags = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	// radius
	// damage_delay
	else
		CBaseMonster::KeyValue( pkvd );
}

void CMonsterMaker::Spawn()
{
	//ALERT( at_console, "CMonsterMaker::Spawn\n");
	m_fIsWarpBall = !strcmp(STRING(pev->classname), "env_warpball");

	pev->solid = SOLID_NOT;

	// for WarpBall to function correctly - it's spawnflag is another then same in monstermaker
	if ( m_fIsWarpBall && FBitSet ( pev->spawnflags, SF_WARPBALL_ONCE )) // flag bit 1
		SetBits ( pev->spawnflags, SF_MONSTERMAKER_FIREONCE );			 // flag bit 16

	m_cLiveChildren = 0;
	Precache();
	if( !FStringNull( pev->targetname ) )
	{
		if( pev->spawnflags & SF_MONSTERMAKER_CYCLIC )
		{
			SetUse( &CMonsterMaker::CyclicUse );// drop one monster each time we fire
		}
		else
		{
			SetUse( &CMonsterMaker::ToggleUse );// so can be turned on/off
		}

		if( !m_fIsWarpBall && FBitSet( pev->spawnflags, SF_MONSTERMAKER_START_ON ))
		{
			// start making monsters as soon as monstermaker spawns
			m_fActive = TRUE;
			SetThink( &CMonsterMaker::MakerThink );
		}
		else
		{
			// wait to be activated.
			m_fActive = FALSE;
			SetThink( &CBaseEntity::SUB_DoNothing );
		}
	}
	else
	{
		// no targetname, just start.
		pev->nextthink = gpGlobals->time + m_flDelay;
		m_fActive = TRUE;
		SetThink( &CMonsterMaker::MakerThink );
	}

	if( m_cNumMonsters == 1 )
	{
		m_fFadeChildren = FALSE;
	}
	else
	{
		m_fFadeChildren = TRUE;
	}

	m_flGround = 0;
}

void CMonsterMaker::Precache( void )
{
	//ALERT( at_console, "%s::Precache\n", STRING(pev->classname));

	CBaseMonster::Precache();

	if (m_fIsWarpBall)
	{
		m_flDelay = 5;
		UTIL_PrecacheOther( "effect_warpball" );
	}

	if (FStringNull( m_iszMonsterClassname ) != true)
	UTIL_PrecacheOther( STRING( m_iszMonsterClassname ) );
	else
		ALERT( at_console, "CMonsterMaker without a children name!\n");
}

//=========================================================
// MakeMonster-  this is the code that drops the monster
//=========================================================
void CMonsterMaker::MakeMonster( void )
{
	edict_t	*pent;
	entvars_t *pevCreate;

	if( m_iMaxLiveChildren > 0 && m_cLiveChildren >= m_iMaxLiveChildren )
	{
		// not allowed to make a new one yet. Too many live ones out right now.
		return;
	}

	bool bFoundTarget = false;
	Vector DesiredOrigin;
	Vector DesiredAngles;
	if (!FStringNull( m_iszWarpTarget ))
	{
		m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( m_iszWarpTarget ) );
		if (m_pGoalEnt)
		{
			DesiredOrigin = m_pGoalEnt->pev->origin;
			DesiredAngles = m_pGoalEnt->pev->angles;
			bFoundTarget = true;
		}
	}

	if (!bFoundTarget)
	{
		DesiredOrigin = pev->origin;
		DesiredAngles = pev->angles;
	}

	if( !m_flGround )
	{
		// set altitude. Now that I'm activated, any breakables, etc should be out from under me. 
		TraceResult tr;

		UTIL_TraceLine( DesiredOrigin, DesiredOrigin - Vector ( 0, 0, 2048 ), ignore_monsters, ENT(pev), &tr );
		m_flGround = tr.vecEndPos.z;
	}

	Vector mins = DesiredOrigin - Vector( 34, 34, 0 );
	Vector maxs = DesiredOrigin + Vector( 34, 34, 0 );
	maxs.z = DesiredOrigin.z;
	mins.z = m_flGround;

	CBaseEntity *pList[2];
	int count = UTIL_EntitiesInBox( pList, 2, mins, maxs, FL_CLIENT | FL_MONSTER );
	if( count )
	{
		// don't build a stack of monsters!
		bool bAllDead = true;
		for ( int i = 0; i < count; i++ )
		{
			if ( pList[i]->IsAlive() )	// Don't count dead monsters
				bAllDead = false;
		}
		// don't build a stack of monsters if there are alive monsters nearby!
		if (!bAllDead)
			return;
	}

	// If env_warpball then create teleport effect
	if ( m_fIsWarpBall == true)
	{
		CEnvWarpBall *pWarpBall = CEnvWarpBall::WarpBallCreate();
		pWarpBall->pev->origin = DesiredOrigin;
		pWarpBall->pev->angles = DesiredAngles;
		SetBits( pWarpBall->pev->spawnflags, SF_AUTO_FIREONCE );
		pWarpBall->Use( this, this, USE_ON, 1);

		// if monstermaker is a warpball and doesn't have children class specified, play effect only
		if (FStringNull(m_iszMonsterClassname))
 			return;
 	}

	pent = CREATE_NAMED_ENTITY( m_iszMonsterClassname );

	if( FNullEnt( pent ) )
	{
		ALERT ( at_console, "NULL Ent in MonsterMaker!\n" );
		return;
	}

	// If I have a target, fire!
	if( !FStringNull( pev->target ) )
	{
		// delay already overloaded for this entity, so can't call SUB_UseTargets()
		FireTargets( STRING( pev->target ), this, this, USE_TOGGLE, 0 );
	}

	pevCreate = VARS( pent );
	pevCreate->origin = DesiredOrigin;
	pevCreate->angles = DesiredAngles;
	pevCreate->spawnflags = m_iChildrenSpawnflags;
	SetBits( pevCreate->spawnflags, SF_MONSTER_FALL_TO_GROUND );

	// Children hit monsterclip brushes
	if( pev->spawnflags & SF_MONSTERMAKER_MONSTERCLIP )
		SetBits( pevCreate->spawnflags, SF_MONSTER_HITMONSTERCLIP );

	DispatchSpawn( ENT( pevCreate ) );
	pevCreate->owner = edict();

	if( !FStringNull( pev->netname ) )
	{
		// if I have a netname (overloaded), give the child monster that name as a targetname
		pevCreate->targetname = pev->netname;
	}

	m_cLiveChildren++;// count this monster
	m_cNumMonsters--;

	if( m_cNumMonsters == 0 )
	{
		// FIXME: do we need this comment? "Disable this forever.  Don't kill it because it still gets death notices"
		//m_cNumMonsters = m_cTotalMonstersCount;
		//m_fActive = FALSE;
		SetThink( NULL );
		SetUse( NULL );
	}

	if ( !FBitSet ( pev->spawnflags, SF_MONSTERMAKER_CYCLIC ) )
	{
		if ( FBitSet ( pev->spawnflags, SF_MONSTERMAKER_FIREONCE ) )
		{
			//ALERT( at_console, "Removing MakeMonster\n");
			UTIL_Remove( this );
		}
	}
}

//=========================================================
// CyclicUse - drops one monster from the monstermaker
// each time we call this.
//=========================================================
void CMonsterMaker::CyclicUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	MakeMonster();
}

//=========================================================
// ToggleUse - activates/deactivates the monster maker
//=========================================================
void CMonsterMaker::ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !ShouldToggle( useType, m_fActive ) )
		return;

	if( m_fActive )
	{
		m_fActive = FALSE;
		SetThink( NULL );
	}
	else
	{
		m_fActive = TRUE;
		SetThink( &CMonsterMaker::MakerThink );
	}

	pev->nextthink = gpGlobals->time;
}

//=========================================================
// MakerThink - creates a new monster every so often
//=========================================================
void CMonsterMaker::MakerThink( void )
{
	//ALERT( at_console, "CMonsterMaker::MakerThink\n");

	pev->nextthink = gpGlobals->time + m_flDelay;

	MakeMonster();
}

//=========================================================
//=========================================================
void CMonsterMaker::DeathNotice( entvars_t *pevChild )
{
	// ok, we've gotten the deathnotice from our child, now clear out its owner if we don't want it to fade.
	m_cLiveChildren--;

	if( !m_fFadeChildren )
	{
		pevChild->owner = NULL;
	}
}

void CMonsterMaker :: MonsterMakerInit( const char* ChildName, int MaxLiveChildren, int NumMonsters )
{
	m_cNumMonsters = NumMonsters;
	m_iMaxLiveChildren = MaxLiveChildren;
	m_iszMonsterClassname = MAKE_STRING( ChildName );
	Spawn();
}

CMonsterMaker *CMonsterMaker::MonsterMakerCreate( const char* ChildName, int MaxLiveChildren, int NumMonsters )
{
	CMonsterMaker *pMonsterMaker = GetClassPtr( (CMonsterMaker *)NULL );
	pMonsterMaker->MonsterMakerInit( ChildName, MaxLiveChildren, NumMonsters );
	return pMonsterMaker;
}
