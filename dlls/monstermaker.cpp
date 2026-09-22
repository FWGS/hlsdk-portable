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
#include "weapons.h"
#include "player.h"

// Monstermaker spawnflags
#define	SF_MONSTERMAKER_START_ON	1 // start active ( if has targetname )
#define	SF_MONSTERMAKER_STUCK		2
#define	SF_MONSTERMAKER_CYCLIC		4 // drop one monster every time fired.
#define SF_MONSTERMAKER_MONSTERCLIP	8 // Children are blocked by monsterclip
#define SF_MONSTERMAKER_PATH_RUN	 16
#define SF_MONSTERMAKER_CORPSE_FADE	 32
#define SF_MONSTERMAKER_KILL_MYSELF	 64
#define SF_MONSTERMAKER_NOLIMIT		128
#define SF_MONSTERMAKER_UNSEE		256
#define SF_MONSTERMAKER_UNSEE2		512
#define SF_MONSTERMAKER_TELEPORT	1024

//=========================================================
// MonsterMaker - this ent creates monsters during the game.
//=========================================================
class CMonsterMaker : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData* pkvd);
	void EXPORT ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT CyclicUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT MakerThink( void );
	void DeathNotice( entvars_t *pevChild );// monster maker children use this to tell the monster maker that they have died.
	void MakeMonster( void );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];
	
	string_t m_iszMonsterClassname;// classname of the monster(s) that will be created.
	
	int m_cNumMonsters;// max number of monsters this ent can create
	
	int m_cLiveChildren;// how many monsters made by this monster maker that are currently alive
	int m_iMaxLiveChildren;// max number of monsters that this maker may have out at one time.

	float m_flGround; // z coord of the ground under me, used to make sure no monsters are under the maker when it drops a new child

	//int m_defuck;
	int m_hlsk;
	int m_waitmulti;
	int m_waitmulti2;

	BOOL m_fActive;
	BOOL m_fFadeChildren;// should we make the children fadeout?
};

LINK_ENTITY_TO_CLASS( monstermaker, CMonsterMaker )

TYPEDESCRIPTION	CMonsterMaker::m_SaveData[] =
{
	DEFINE_FIELD( CMonsterMaker, m_iszMonsterClassname, FIELD_STRING ),
	DEFINE_FIELD( CMonsterMaker, m_cNumMonsters, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_cLiveChildren, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_waitmulti, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_waitmulti2, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_hlsk, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_flGround, FIELD_FLOAT ),
	DEFINE_FIELD( CMonsterMaker, m_iMaxLiveChildren, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_fActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMonsterMaker, m_fFadeChildren, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CMonsterMaker, CBaseMonster )

void CMonsterMaker::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "monstercount" ) )
	{
		m_cNumMonsters = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "m_imaxlivechildren" ) )
	{
		m_iMaxLiveChildren = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "m_hlsk") )
	{
		m_hlsk = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "monstertype" ) )
	{
		m_iszMonsterClassname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

void CMonsterMaker::Spawn()
{
	pev->solid = SOLID_NOT;

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

		if( FBitSet( pev->spawnflags, SF_MONSTERMAKER_START_ON ) )
		{
			// start making monsters as soon as monstermaker spawns
			pev->nextthink = gpGlobals->time + m_flDelay;
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

	if ( m_cNumMonsters <= -1 )
	{
		m_cNumMonsters = 20;
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
	CBaseMonster::Precache();

	UTIL_PrecacheOther( STRING( m_iszMonsterClassname ) );
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
		m_waitmulti = 1;
		return;
	}

	if( !m_flGround )
	{
		// set altitude. Now that I'm activated, any breakables, etc should be out from under me. 
		TraceResult tr;

		UTIL_TraceLine( pev->origin, pev->origin - Vector( 0, 0, 2048 ), ignore_monsters, ENT( pev ), &tr );
		m_flGround = tr.vecEndPos.z;
	}

	Vector mins = pev->origin - Vector( 34, 34, 0 );
	Vector maxs = pev->origin + Vector( 34, 34, 0 );
	maxs.z = pev->origin.z;
	mins.z = m_flGround;

	CBaseEntity *pList[2];
	int count = UTIL_EntitiesInBox( pList, 2, mins, maxs, FL_CLIENT | FL_MONSTER );
	if( count )
	{
		if ( !(pev->spawnflags & SF_MONSTERMAKER_STUCK) )
		{

			if( m_hlsk == 364 )
			{
				if(FStrEq(STRING(pev->targetname), "hecu_follower"))
				{
					pev->origin.x += 32;
				}
			}
			return;
		}
		return;
	}


	if( pev->spawnflags & SF_MONSTERMAKER_UNSEE )
	{
		if ( FStrEq( STRING(m_iszMonsterClassname ), "sickbolt" ) )
		{
			if ( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
				return;
		}
		else
		{
			if ( !FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
				return;
		}
	}

	if( pev->spawnflags & SF_MONSTERMAKER_TELEPORT )
	{
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY,pev->origin);
		WRITE_BYTE(3);
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z);
		WRITE_SHORT(g_sModelIndexCteleport);
		WRITE_BYTE(15);
		WRITE_BYTE(15);
		WRITE_BYTE(4);
		MESSAGE_END();
		
		EMIT_SOUND_DYN( ENT(pev), CHAN_AUTO, "debris/beamstart2old.wav", 1, ATTN_NORM, 0, 100 );
	}

	if( (pev->spawnflags & SF_MONSTERMAKER_UNSEE2) )
	{
		CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pEntity )
		{
			CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
			if(pPlayer)
			{
				if(pPlayer->FVisible( this ))
				{
					if ( pPlayer->FInViewCone (this))
					{
						pev->nextthink = gpGlobals->time + 1;
						return;
					}
				}
			}
		}

		if ( m_iMaxLiveChildren == 1 )
		pev->nextthink = gpGlobals->time + 9999;
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
	if(m_hlsk == 363)
	{
		int rd = RANDOM_LONG(0,2);
		if(rd == 0)
		{
			pevCreate->origin = pev->origin;
		}
		else if(rd == 1)
		{
			pevCreate->origin = pev->origin + Vector(0,256,0);
		}
		else if(rd == 2)
		{
			pevCreate->origin = pev->origin - Vector(0,256,0);
		}
	}
	else
	{
		pevCreate->origin = pev->origin;
	}
	pevCreate->angles = pev->angles;
	SetBits( pevCreate->spawnflags, SF_MONSTER_FALL_TO_GROUND );

	// Children hit monsterclip brushes
	if( pev->spawnflags & SF_MONSTERMAKER_MONSTERCLIP )
		SetBits( pevCreate->spawnflags, SF_MONSTER_HITMONSTERCLIP );

	DispatchSpawn( ENT( pevCreate ) );

	if ( !(pev->spawnflags & SF_MONSTERMAKER_KILL_MYSELF) )
		pevCreate->owner = edict();
	
	CBaseEntity *pEntity = GetClassPtr((CBaseEntity *)pevCreate);
	CBaseMonster *pEnemyMonster;
	if(pEntity)
	{
		pEnemyMonster = pEntity->MyMonsterPointer();
	}

	if(pev->weapons != 0)
	{
		pevCreate->weapons = pev->weapons;
		if(pEnemyMonster)
		{
			pEnemyMonster->m_makerspawn_call = 1;
		}
	}
	if(pev->body != 0)
	{
		pevCreate->body = pev->body;
		if(pEnemyMonster)
		{
			pEnemyMonster->m_makerspawn_call = 1;
		}
	}
	if(pev->frags != 0)
	{
		pevCreate->frags = pev->frags;
	}
	if(pev->team != 0)
	{
		pevCreate->team = pev->team;
	}
	if(pev->skin != 0)
	{
		pevCreate->skin = pev->skin;
	}
	if(pev->flags != 0)
	{
		pevCreate->spawnflags = pev->flags;
	}
	if(pev->armortype != 0)
	{
		pevCreate->armortype = pev->armortype;
	}
	if(pev->impulse != 0)
	{
		pevCreate->impulse = pev->impulse;
	}
	if(pev->health != 0)
	{
		pevCreate->health = pev->health;
	}
	if( pev->spawnflags & SF_MONSTERMAKER_PATH_RUN )
	{
		pevCreate->iuser1 = 1;
	}
	
	if( m_hlsk == 484 )
	{
		if ( !strcmp( STRING( gpGlobals->mapname ), "c0a0_wdoor") )
		{
			pEntity->pev->origin.y = pEntity->pev->origin.y + 20;
		}
		FX_Explosion(pEntity->Center(), EXPLOSION_DISPTELEPORT );
	}

	if(pEnemyMonster)
	{//My Monster Connect
		if( pev->spawnflags & SF_MONSTERMAKER_CORPSE_FADE )
		{
			pEnemyMonster->m_diefadeout = 1;
		}
		if( pev->spawnflags & SF_MONSTERMAKER_NOLIMIT )
		{
			pEnemyMonster->m_no_pov_limit = 1;
		}
		if( m_hlsk == 111 )
		{
			if ( FClassnameIs( pEntity->pev, "monster_massn" ) )
			{
				pEnemyMonster->m_running_rangeattack = 1;
				pEnemyMonster->pev->weapons = 1;
				pEnemyMonster->SetBodygroup( 2, 0 );
				pEnemyMonster->m_cClipSize		= 30;
				pEnemyMonster->m_cAmmoLoaded	= 30;
				pEnemyMonster->m_FTSmod = 3;
				DROP_TO_FLOOR ( ENT(pEnemyMonster->pev) );
			}
		}
		if( m_hlsk == 114 )
		{
			pEnemyMonster->m_lovehate = 810;
		}
		if( m_hlsk == 145 )
		{
			pEnemyMonster->m_alert = 100;
			pEnemyMonster->pev->flags |= FL_FROZEN;
			pEnemyMonster->m_notarget_hide = 30;		
		}
		if( m_hlsk == 155 )
		{
			pEnemyMonster->m_alert = 100;
		}
		if( m_hlsk == 159 )
		{
			pEnemyMonster->m_alert = 100;
			pEnemyMonster->m_lovehate = 810;
			pEnemyMonster->pev->flags |= FL_FROZEN;
			pEnemyMonster->m_notarget_hide = 40;
		}
		if( m_hlsk == 175 )
		{
			pEnemyMonster->m_ignoreFail = 1919;
			pEnemyMonster->m_alwaysrunpath = TRUE;
		}
		if( m_hlsk == 200)
		{
			pEnemyMonster->m_FTSmod = 8;
			pEnemyMonster->m_alert = 100;
		}
		if( m_hlsk == 201)
		{
			pEnemyMonster->m_alert = 100;
			pEnemyMonster->m_ignoreFail_MAX = 20;
			pEnemyMonster->m_ignoreFail_OFF = 0;
			pEnemyMonster->m_MoveFail_FuckRoad = TRUE;
			pEnemyMonster->m_MoveFail_SimpleRoad = TRUE;
		}
		if( m_hlsk == 214)
		{
			pevCreate->spawnflags |= SF_MONSTER_GAG;
			pevCreate->spawnflags |= SF_MONSTER_PRISONER;
		}
		if( m_hlsk == 333 )
		{
			pEnemyMonster->m_FTSmod = 3;
		}
		if( m_hlsk == 334 )
		{
			pEnemyMonster->m_FTSmod = 3;
			pEnemyMonster->m_ctmod = 1;
			pEnemyMonster->pev->flags |= FL_MONSTERCLIP;
		}
		if( m_hlsk == 335 )
		{
			pEnemyMonster->m_FTSmod = 3;
			pEnemyMonster->m_ctmod = 1;
			pEnemyMonster->m_ignoreFail_OFF = 1;
			pEnemyMonster->m_chase_mode = 0;
			pEnemyMonster->pev->flags |= FL_MONSTERCLIP;
		}
		if( m_hlsk == 336 )
		{
			pEnemyMonster->m_longming = 1;
			pEnemyMonster->pev->takedamage = DAMAGE_NO;
			pEnemyMonster->m_no_pov_limit = 1;
		}
		if( m_hlsk == 337 )
		{
			pEnemyMonster->m_lovehate = 1;
			pEnemyMonster->m_no_pov_limit = 1;
		}
		if( m_hlsk == 338 )
		{
			pEnemyMonster->pev->flags |= FL_FROZEN;
			pEnemyMonster->m_notarget_hide = 20;
			pEnemyMonster->m_iTriggerCondition = 4;
			pEnemyMonster->m_walkaround = TRUE;
			pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("monster_killed_count1");
			pEnemyMonster->m_alert = 100;
		}
		if( m_hlsk == 339 )
		{
			pEnemyMonster->m_iTriggerCondition = 4;
			pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("monster_killed_count1");
		}
		if( m_hlsk == 341 )
		{
			pEnemyMonster->m_iTriggerCondition = 4;
			pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("barney_tr1_dead");
		}
		if( m_hlsk == 342 )
		{
			pEnemyMonster->m_iTriggerCondition = 4;
			pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("barney_tr2_dead");
		}
		if( m_hlsk == 343 )
		{
			pEnemyMonster->m_iTriggerCondition = 4;
			pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("tr3_kill_rabbit");
		}
		if( m_hlsk == 344 )
		{
			pEnemyMonster->m_iTriggerCondition = 4;
			pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("tr3_kill_thelast");
		}
		if( m_hlsk == 345)
		{
			pEnemyMonster->m_walkaround = TRUE;
		}
		if( m_hlsk == 346)
		{
			pEnemyMonster->m_FTSmod = 3;
			pEnemyMonster->m_diefadeout = 1;
		}
		if( m_hlsk == 347)
		{
			pEnemyMonster->m_FTSmod = 3;
			pEnemyMonster->m_diefadeout = 1;
			pEnemyMonster->m_die_corpse_solid = 1;
		}
		if( m_hlsk == 348 )
		{
			pEnemyMonster->m_alert = 100;
			pEnemyMonster->m_alwaysrunpath = TRUE;
		}
		if( m_hlsk == 349)
		{
			MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY,pev->origin);
			WRITE_BYTE(3);
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z);
			WRITE_SHORT(g_sModelIndexCteleport);
			WRITE_BYTE(15);
			WRITE_BYTE(15);
			WRITE_BYTE(4);
			MESSAGE_END();
			
			EMIT_SOUND_DYN( ENT(pev), CHAN_AUTO, "debris/beamstart2old.wav", 1, ATTN_NORM, 0, 100 );
		}
		if(m_hlsk == 350)
		{
			pEnemyMonster->m_alert = 100;
			pEnemyMonster->pev->flags |= FL_FROZEN;
			pEnemyMonster->m_notarget_hide = 5;
			pEnemyMonster->m_MoveFail_SimpleRoad = TRUE;
			pEnemyMonster->m_ignoredamage = 1;
			pEnemyMonster->m_EyeMod = 1;
			pEnemyMonster->m_no_pov_limit = 1;
			pEnemyMonster->m_ignorePlayer = 40;
			pEnemyMonster->m_ignoreFail_MAX = 30;
			pEnemyMonster->m_ignoreFail_OFF = 0;
			pEnemyMonster->m_forcefuckdoor  = TRUE;
			pEnemyMonster->pev->impulse = 1;
		}
		if( m_hlsk == 351)
		{
			MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY,pev->origin);
			WRITE_BYTE(3);
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z);
			WRITE_SHORT(g_sModelIndexCteleport);
			WRITE_BYTE(15);
			WRITE_BYTE(15);
			WRITE_BYTE(4);
			MESSAGE_END();
			
			EMIT_SOUND_DYN( ENT(pev), CHAN_AUTO, "debris/beamstart2old.wav", 1, ATTN_NORM, 0, 100 );

			pEnemyMonster->m_alert = 100;
			pEnemyMonster->m_walkaround = TRUE;
			pEnemyMonster->m_alwaysrunpath = TRUE;
			pEnemyMonster->m_no_pov_limit = 1;
			pEnemyMonster->m_MoveFail_SimpleRoad = TRUE;
			pEnemyMonster->m_selfmode = TRUE;
			pEnemyMonster->m_chase_mode = 0;
		}
		if( m_hlsk == 352 )
		{
			pEnemyMonster->m_alert = 200;
			pEnemyMonster->m_walkaround = TRUE;
			pEnemyMonster->m_alwaysrunpath = TRUE;
			pEnemyMonster->m_no_pov_limit = 1;
			pEnemyMonster->m_diefadeout = 1;
		}
		if( m_hlsk == 358)
		{
			pEnemyMonster->m_FTSmod = 7;
			pEnemyMonster->m_diefadeout = 1;
		}
		if( m_hlsk == 359)
		{
			pEnemyMonster->m_walkaround = TRUE;
			pEnemyMonster->m_walkaroundFail = TRUE;
			pEnemyMonster->m_alert = 100;
			pEnemyMonster->m_diefadeout = 1;
		}
		if( m_hlsk == 360)
		{
			pEnemyMonster->m_FTSmod = 3;
			pEnemyMonster->m_alert = 100;
			pEnemyMonster->m_selfmode = TRUE;
			pEnemyMonster->m_no_pov_limit = 1;
		}
		if( m_hlsk == 361 )
		{
			pEnemyMonster->m_alert = 100;
			pEnemyMonster->m_no_pov_limit = 1;
			pEnemyMonster->m_ignoreFail = 1919;
			pEnemyMonster->m_MoveFail_FuckRoad = TRUE;
		}
		if( m_hlsk == 363 )
		{
			if ( FClassnameIs( pEntity->pev, "monster_human_grunt" ) )
			{
				pEnemyMonster->pev->weapons = 16;
				pEnemyMonster->SetBodygroup( 2, 2 );
				pEnemyMonster->m_cClipSize = 20;
				pEnemyMonster->m_cAmmoLoaded = 20;
				pEnemyMonster->m_no_pov_limit = 1;
				pEnemyMonster->m_undropgun = TRUE;
			}
		}
		if( m_hlsk == 364 )
		{
			pEnemyMonster->m_FTSmod = 8;
			pEnemyMonster->m_alert = 100;
			pEnemyMonster->m_no_pov_limit = 1;
		}
		if( m_hlsk == 365 )
		{
			pEnemyMonster->m_FTSmod = 3;
			pEnemyMonster->m_alert = 100;
			pEnemyMonster->m_diefadeout = 1;
			pEnemyMonster->m_iTriggerCondition = 4;
			pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("wroad_duct_killed");
		}
		if( m_hlsk == 369 )
		{
			pEnemyMonster->m_FTSmod = 3;
			pEnemyMonster->m_alert = 100;
			pEnemyMonster->m_diefadeout = 1;
			pEnemyMonster->m_iTriggerCondition = 4;
			pEnemyMonster->m_boltpoison = 10;
			pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("gh_hecu_killed");
		}
		if( m_hlsk == 370 )
		{
			pEnemyMonster->m_alert = 100;
			pEnemyMonster->m_walkaround = TRUE;
			pEnemyMonster->m_walkaroundFail = TRUE;
			pEnemyMonster->m_diefadeout = 1;
		}
		if( m_hlsk == 89 )
		{
			pEnemyMonster->m_alert = 100;
			pEnemyMonster->m_diefadeout = 1;
			pEnemyMonster->m_iTriggerCondition = 4;
			pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("wisemajo_killed");
		}
		if( m_hlsk == 90 )
		{
			pEnemyMonster->pev->spawnflags = SF_MONSTER_PRISONER;
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupSequence( "head" );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
		}
		if( m_hlsk == 91 )
		{
			pEnemyMonster->m_alert = 100;
			pEnemyMonster->m_iTriggerCondition = 4;
			pEnemyMonster->m_boltpoison = 30;
			pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("killed_chainsawboss");
		}
		if( m_hlsk == 92 )
		{
			pEnemyMonster->m_iTriggerCondition = 4;
			pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("killed_wddman");
		}
		if( m_hlsk == 456 )
		{
			pEnemyMonster->m_FTSmod = 1;
			pEnemyMonster->m_diefadeout = 1;
			pEnemyMonster->m_no_pov_limit = 1;
			pEnemyMonster->m_ctmod = 2;
			pEnemyMonster->m_forcefuckdoor = FALSE;
			pEnemyMonster->m_EyeMod = 1;
			pEnemyMonster->m_flDistLook	= 6000.0;
			pEnemyMonster->m_boltpoison = 10;
		}
		if( m_hlsk == 457 )
		{
			pEnemyMonster->m_FTSmod = 1;
			pEnemyMonster->m_no_pov_limit = 1;
			pEnemyMonster->m_diefadeout = 1;
			pEnemyMonster->m_ctmod = 2;
			pEnemyMonster->m_forcefuckdoor = FALSE;
			pEnemyMonster->m_rpgms_inteam = 5;
			pEnemyMonster->m_boltpoison = 20;
			pEnemyMonster->m_selfmode = TRUE;
			pEnemyMonster->m_flDistLook	= 6000.0;
		}
		if( m_hlsk == 388)
		{
			pEnemyMonster->m_walkaround = TRUE;
			pEnemyMonster->m_walkaroundFail = TRUE;
			pEnemyMonster->m_alwaysrunpath = TRUE;
			pEnemyMonster->m_alert = 100;
		}
		if( m_hlsk == 467 )
		{
			pEnemyMonster->pev->spawnflags = SF_MONSTER_PRISONER;
			pEnemyMonster->pev->sequence = pEnemyMonster->LookupActivity ( ACT_COWER );
			pEnemyMonster->ResetSequenceInfo( );
			pEnemyMonster->pev->frame = 0;
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
			pEnemyMonster->pev->takedamage = DAMAGE_NO;
			pEnemyMonster->m_groundElev = TRUE;
			pEnemyMonster->pev->movetype = MOVETYPE_BOUNCE;
			pEnemyMonster->pev->friction = 1.0;
		}
		if( m_hlsk == 468 )
		{
			pEnemyMonster->m_FTSmod = 8;
			pEnemyMonster->m_alert = 100;
			pEnemyMonster->m_no_pov_limit = 1;
			pEnemyMonster->m_diefadeout = 1;
			pEnemyMonster->m_aimflag_dist = 64.0;
			pEnemyMonster->m_EyeMod = 1;
			if ( RANDOM_LONG(0,31) < 8 )
			{
				pEnemyMonster->m_elseuseful = 2;
			}
			else
			{
				pEnemyMonster->m_elseuseful = 1;
			}
			pEnemyMonster->pev->skin = RANDOM_LONG(0,2);
			pEnemyMonster->m_undropgun = TRUE;
		}
		if( m_hlsk == 470 )
		{
			pEnemyMonster->m_iTriggerCondition = 4;
			pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("monster_killed_count1");
			pEnemyMonster->m_diefadeout = 1;
		}
		if( m_hlsk == 471 )
		{
			pEnemyMonster->m_iTriggerCondition = 4;
			pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("monster_killed_count2");
			pEnemyMonster->m_diefadeout = 1;
		}
		if( m_hlsk == 472 )
		{
			pEnemyMonster->SetState( MONSTERSTATE_HUNT );
			pEnemyMonster->m_groundElev = TRUE;
			pEnemyMonster->pev->movetype = MOVETYPE_BOUNCE;
		}
		if( m_hlsk == 473 )
		{
			pEnemyMonster->m_diefadeout = 1;
			pEnemyMonster->m_undropgun = TRUE;
		}
		if( m_hlsk == 474 )
		{
			pEnemyMonster->m_iTriggerCondition = 4;
			pEnemyMonster->m_iszTriggerTarget = MAKE_STRING("monster_killed_trigger1");
			pEnemyMonster->m_diefadeout = 1;
			pEnemyMonster->m_boltpoison = 30;
		}
		if( m_hlsk == 864 )
		{
			pevCreate->takedamage = DAMAGE_NO;
			pevCreate->spawnflags |= SF_MONSTER_PRISONER;
		}
	}

	
	if ( !FStringNull( pev->message ) )
	{
		pevCreate->target = pev->message;
	}

	if( !FStringNull( pev->netname ) )
	{
		// if I have a netname (overloaded), give the child monster that name as a targetname
		pevCreate->targetname = pev->netname;
	}

	m_cLiveChildren++;// count this monster


	if ( !FStrEq( STRING(m_iszMonsterClassname ), "monster_meat_hos" )
	&& !FStrEq( STRING(m_iszMonsterClassname ), "monster_meat_hos_dead" )
	&& !FStrEq( STRING(m_iszMonsterClassname ), "sickbolt" )
	&& !FStrEq( STRING(m_iszMonsterClassname ), "monster_schoolgirl" ))
		m_cNumMonsters--;
	
	if( m_cNumMonsters == 0 )
	{
		// Disable this forever.  Don't kill it because it still gets death notices
		SetThink( NULL );
		SetUse( NULL );
	}

	if ( pev->spawnflags & SF_MONSTERMAKER_KILL_MYSELF )
	{
		UTIL_Remove(this);
		return;
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

	if( pev->spawnflags & SF_MONSTERMAKER_UNSEE2 )
	{
		if ( m_iMaxLiveChildren == 1 )
		pev->nextthink = gpGlobals->time + m_flDelay;
	}
}
