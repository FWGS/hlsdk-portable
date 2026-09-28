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
// Generic Monster - purely for scripted sequence work.
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"player.h"

// For holograms, make them not solid so the player can walk through them
#define	SF_GENERICMONSTER_NOTSOLID					4 

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CGenericMonster : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int Classify( void );
	void RunAI( void );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int ISoundMask( void );
};

LINK_ENTITY_TO_CLASS( monster_generic, CGenericMonster )

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CGenericMonster::Classify( void )
{
	return CLASS_NONE;
}

void CGenericMonster :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if (pev->sequence == LookupActivity ( ACT_WALK ))
	{
		if ( pev->weapons == 167 )
		{
			m_flGroundSpeed = 90;
		}
	}
}

int CGenericMonster :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if(pev->frags == 1000)
	{
		if(!FBitSet(pev->effects, EF_DIMLIGHT))
		{
			pev->renderfx = 0;
			pev->rendermode = 0;
			SetBits(pev->effects, EF_DIMLIGHT);
			UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, Vector( 16, 16, 48 ));
		}
		return 0;
	}
	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGenericMonster::SetYawSpeed( void )
{
	int ys;

	switch( m_Activity )
	{
	case ACT_IDLE:
	default:
		ys = 90;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGenericMonster::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 0:
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CGenericMonster::ISoundMask( void )
{
	return 0;
}

//=========================================================
// Spawn
//=========================================================
void CGenericMonster::Spawn()
{
	Precache();

	SET_MODEL( ENT( pev ), STRING( pev->model ) );
/*
	if( FStrEq( STRING( pev->model ), "models/player.mdl" ) )
		UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
	else
		UTIL_SetSize( pev, VEC_HULL_MIN, VEC_HULL_MAX);
*/
	if( FStrEq( STRING( pev->model ), "models/player.mdl" ) || FStrEq( STRING( pev->model ), "models/holo.mdl" ) )
		UTIL_SetSize( pev, VEC_HULL_MIN, VEC_HULL_MAX );
	else
		UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	if(pev->health == 0)
	{
		pev->health = 8;
	}
	if(pev->frags == 1000)
	{
		UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, Vector( 16, 16, 48 ));
		m_bloodColor		= DONT_BLEED;
	}
	m_flFieldOfView = 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();

	if( pev->spawnflags & SF_GENERICMONSTER_NOTSOLID )
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
		pev->flags |= FL_NOTARGET;
	}

	if ( pev->weapons == 168 )
	{
		pev->takedamage = DAMAGE_NO;
		pev->flags|= FL_NOTARGET;
		pev->solid = SOLID_NOT;
		pev->rendermode = kRenderTransAdd;
		pev->renderfx = kRenderFxHologram;
		pev->renderamt = 255;
	}

	if ( pev->weapons == 255 )
	{
		pev->takedamage = DAMAGE_NO;
		pev->spawnflags |= SF_MONSTER_PRISONER;
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGenericMonster::Precache()
{
	PRECACHE_MODEL( STRING( pev->model ) );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

class CDeadGeneric : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
};

LINK_ENTITY_TO_CLASS( monster_generic_dead, CDeadGeneric );

//=========================================================
// ********** DeadBarney SPAWN **********
//=========================================================
void CDeadGeneric :: Spawn( )
{
	Precache();

	SET_MODEL( ENT(pev), STRING(pev->model) );

	pev->effects = 0;
	pev->yaw_speed = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	if ( pev->spawnflags == 1 )
	{
		m_bloodColor = BLOOD_COLOR_YELLOW;
	}
	else if ( pev->spawnflags == 2 )
	{
		m_bloodColor = DONT_BLEED;
	}
	
	InitBoneControllers();

	pev->solid = SOLID_BBOX;

	pev->frame = 255;
	//ResetSequenceInfo( );
	pev->framerate = 0;
	
	// Copy health
	pev->max_health	= pev->health;
	pev->deadflag = DEAD_DEAD;
	
	UTIL_SetSize(pev, g_vecZero, g_vecZero );
	UTIL_SetOrigin( pev, pev->origin );

	// Setup health counters, etc.
	BecomeDead();
	pev->movetype = MOVETYPE_TOSS;// so he'll fall to ground
	pev->takedamage = DAMAGE_YES;

	if ( pev->spawnflags == 4 )
	{
		m_bloodColor = DONT_BLEED;
		pev->takedamage = DAMAGE_NO;
	}

	SetThink( &CDeadGeneric::CorpseFallThink );
	pev->nextthink = gpGlobals->time + 0.5;
}

int	CDeadGeneric :: Classify ( void )
{
	if(m_bloodColor	== BLOOD_COLOR_RED)
	{
		return	CLASS_PLAYER_ALLY;
	}
	else
	{
		return	CLASS_ALIEN_MONSTER;
	}
}


void CDeadGeneric :: Precache()
{
	PRECACHE_MODEL( (char *)STRING(pev->model) );
}	



class CItemGeneric : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int	Classify ( void ) { return	CLASS_NONE; }

	void  EXPORT fall_think ( void );

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return (CBaseToggle :: ObjectCaps() | FCAP_CONTINUOUS_USE) & ~FCAP_ACROSS_TRANSITION; }
	void EXPORT DefaultTouch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( monster_generic_item, CItemGeneric );
LINK_ENTITY_TO_CLASS( monster_generic_item2, CItemGeneric );

//=========================================================
// ********** DeadBarney SPAWN **********
//=========================================================
void CItemGeneric :: Spawn( )
{
	if ( FClassnameIs(pev,"monster_generic_item") )
	{
		Precache();

		SET_MODEL( ENT(pev), STRING(pev->model) );
	}

	pev->effects = 0;
	pev->yaw_speed = 0;
	m_bloodColor = DONT_BLEED;

	pev->solid = SOLID_TRIGGER;

	pev->frame = 0;
	ResetSequenceInfo( );
	pev->framerate = 0;

	UTIL_SetSize(pev, g_vecZero, g_vecZero );
	UTIL_SetOrigin( pev, pev->origin );

	pev->movetype = MOVETYPE_BOUNCE;

	pev->effects  = EF_DIMLIGHT;

	SetTouch( &CItemGeneric::DefaultTouch );
	pev->nextthink = gpGlobals->time + 0.5;
	SetThink( &CItemGeneric::fall_think );
}

void CItemGeneric :: Precache()
{
	PRECACHE_MODEL( (char *)STRING(pev->model) );
	PRECACHE_SOUND("weapons/ladder_deploy.wav");
}	

void CItemGeneric :: fall_think( void )
{
	if(pev->armorvalue > 0)
	pev->armorvalue -= 1;

	pev->nextthink = gpGlobals->time + 0.5;
		
}

void CItemGeneric::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	// Make sure that we have a caller
	if (!pActivator)
		return;
	// if it's not a player, ignore
	if ( !pActivator->IsPlayer() )
		return;

	if(!FNullEnt(pev->owner))
		return;

	if(pev->armorvalue > 0)
		return;

	CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pActivator->pev);
	if(pPlayer)
	{
		if(pPlayer->m_fMoveItem == NULL)
		{

			if (!FStringNull(pev->target))
			{
				FireTargets( STRING(pev->target), this, this, USE_TOGGLE, 0 );
				pev->target = 0;
			}

			pPlayer->m_fMoveItem = this;
			pev->owner = ENT(pPlayer->pev);
			pev->renderfx = kRenderNormal;
			pev->movetype = MOVETYPE_NONE;

			if ( FClassnameIs(pev,"monster_generic_item") )
			{
				pev->body = 0;
			}
		}
	}

}

void CItemGeneric::DefaultTouch( CBaseEntity *pOther )
{
	if(pev->movetype == MOVETYPE_NONE)
		return;

	if (!(pOther->pev->flags & FL_MONSTER) && !pOther->IsPlayer() && pOther->pev->solid != SOLID_TRIGGER)
	{
		float flSpeed = pev->velocity.Length();
		if (flSpeed >= 350)
		{
			UTIL_Sparks( pev->origin + Vector( RANDOM_FLOAT( -10, 10 ),RANDOM_FLOAT( -10, 10 ),RANDOM_FLOAT( 0, 10 ) ) );
		}
		
        pev->avelocity.x = 0;
        pev->avelocity.y = RANDOM_FLOAT( -flSpeed, flSpeed );
        pev->angles.x = 0;
		pev->angles.z = 0;
			if (flSpeed >= 300)
			{
            	EMIT_SOUND(ENT(pev), CHAN_BODY, "items/weapondrop1.wav", 1, ATTN_NORM);
			}
			else if (flSpeed >= 200)
			{
            	EMIT_SOUND(ENT(pev), CHAN_BODY, "items/weapondrop1.wav", 0.8, ATTN_NORM);
			}
			else if (flSpeed >= 100)
			{
            	EMIT_SOUND(ENT(pev), CHAN_BODY, "items/weapondrop1.wav", 0.6, ATTN_NORM);
			}
			pev->velocity = pev->velocity * 0.4;
	}

	pev->angles.x = pev->angles.z = 0;

	UTIL_MakeVectors ( pev->angles );

	Vector right, forward, up = Vector( 0, 0, 20 );
	TraceResult tr1, tr2;
	float angle_z;

	UTIL_TraceLine ( pev->origin + up, pev->origin - up, ignore_monsters, ENT(pev), &tr1 );
	UTIL_TraceLine ( pev->origin + gpGlobals->v_right * 5 + up, pev->origin - up + gpGlobals->v_right * 5, ignore_monsters, ENT(pev), &tr2 );

	if ( tr1.flFraction < 1.0 && tr2.flFraction < 1.0 )
	{
		up = tr1.vecPlaneNormal;
		right = ( tr2.vecEndPos - pev->origin ).Normalize ();
		forward = CrossProduct ( up, right );

		//hack: acos ranges from 0 to M_PI
		int sgn = ( gpGlobals->v_right.z > right.z ) ? 1 : -1;

		angle_z = ( acos ( DotProduct ( gpGlobals->v_right, right ) ) * 180.0f ) / M_PI;

		pev->angles = UTIL_VecToAngles ( forward );
		pev->angles.z = angle_z * sgn;
	}

	
}

class CLadderItem_point : public CBaseEntity
{
public:
	void	Spawn( void );
	void	EXPORT DrawThink ( void );
};

LINK_ENTITY_TO_CLASS( ladder_item_point, CLadderItem_point );

void CLadderItem_point::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NOCLIP;

	SET_MODEL(ENT(pev), "models/flag.mdl");
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;

	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;

	SetThink (&CLadderItem_point::DrawThink);
	pev->nextthink = gpGlobals->time + 0.5;
}


void CLadderItem_point::DrawThink ( void )
{
	pev->nextthink = gpGlobals->time + 0.5;
	CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
	if ( pEntity )
	{
		float flDist = ( pev->origin - pEntity->pev->origin).Length();
		if(flDist >= 80 + (20 * pev->frags))
			return;

		if (!FVisible( pEntity ))
			return;	

		CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
		if(pPlayer)
		{
				if(pPlayer->m_fMoveItem != NULL)
				{
					if(pPlayer->m_fMoveItem->pev->weapons == pev->weapons && FClassnameIs(pPlayer->m_fMoveItem->pev,"monster_generic_item"))
					{
					
						pPlayer->m_fMoveItem->pev->movetype = MOVETYPE_NONE;
						pPlayer->m_fMoveItem->pev->angles = pev->angles;
						pPlayer->m_fMoveItem->pev->solid = SOLID_NOT;
						pPlayer->m_fMoveItem->pev->body = 1;
						pPlayer->m_fMoveItem->pev->armorvalue = 100;

						if(pev->armortype == 1)
						{	
							pPlayer->m_fMoveItem->pev->origin = pev->origin - Vector(0,0,150);
							pPlayer->m_fMoveItem->pev->body = 2;
							pPlayer->m_fMoveItem->pev->effects = 0;
							FireTargets( STRING(pev->target), this, this, USE_TOGGLE, 0 );
						}
						else
						{
							pPlayer->m_fMoveItem->pev->origin = pev->origin;
						}

						pPlayer->m_fMoveItem->SetThink ( NULL );
						pPlayer->m_fMoveItem->SetTouch ( NULL );
						pPlayer->m_fMoveItem = NULL;
						pev->targetname = 0;

						EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/ladder_deploy.wav", 1.0, ATTN_NORM );

						CBaseEntity *pEntity2 = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );//Ѱ���
						if ( pEntity2 )
						{
							if ( FClassnameIs(pEntity2->pev,"func_ladder") || FClassnameIs(pEntity2->pev,"func_ladder_new") ){
							pEntity2->pev->solid = SOLID_NOT;
							pEntity2->pev->spawnflags = 0;
							}
						}

						SetThink( &CLadderItem_point::SUB_Remove );
						pev->nextthink = pev->ltime + 0.1;
						return;
					}
				}
			}
		}
}