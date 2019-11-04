/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
/*

===== molotov.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"
#include "effects.h"
#include "triggers.h"

#include "monster_molotov.h"

//===================molotov


LINK_ENTITY_TO_CLASS( molotov, CMonsterMolotov );

//TYPEDESCRIPTION	CMonsterMolotov::m_SaveData[] = 
//{
	//DEFINE_FIELD( CMonsterMolotov, m_pFireSprite[0][0], FIELD_CLASSPTR ),
	//DEFINE_FIELD( CMonsterMolotov, m_pFireSprite[1][0], FIELD_CLASSPTR ),
	//DEFINE_FIELD( CMonsterMolotov, m_pFireSprite[2][0], FIELD_CLASSPTR ),
	//DEFINE_FIELD( CMonsterMolotov, m_pFireSprite[0][1], FIELD_CLASSPTR ),
	//DEFINE_FIELD( CMonsterMolotov, m_pFireSprite[1][1], FIELD_CLASSPTR ),
	//DEFINE_FIELD( CMonsterMolotov, m_pFireSprite[2][1], FIELD_CLASSPTR ),
	//DEFINE_FIELD( CMonsterMolotov, m_pFireSprite[0][2], FIELD_CLASSPTR ),
	//DEFINE_FIELD( CMonsterMolotov, m_pFireSprite[1][2], FIELD_CLASSPTR ),
	//DEFINE_FIELD( CMonsterMolotov, m_pFireSprite[2][2], FIELD_CLASSPTR ),
	//DEFINE_ARRAY( CMonsterMolotov, m_pFireSprite, FIELD_CLASSPTR, 9 ),
	//DEFINE_ARRAY( CMonsterMolotov, m_pFireSprite[1], FIELD_CLASSPTR, 3 ),
	//DEFINE_ARRAY( CMonsterMolotov, m_pFireSprite[2], FIELD_CLASSPTR, 3 ),
	//DEFINE_FIELD( CMonsterMolotov, m_pFireHurt, FIELD_CLASSPTR ),
	//DEFINE_FIELD( CMonsterMolotov, m_pBurningArea, FIELD_CLASSPTR ),
//};
//IMPLEMENT_SAVERESTORE( CMonsterMolotov, CBaseMonster );
int CMonsterMolotov::Save( CSave &save )
{
	// get rid of the stuff that doesn't get saved...
	int spr;
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			spr = 3*(x+1)+(y+1);
			UTIL_Remove( m_pFireSprite[spr] );
			m_pFireSprite[spr] = NULL;
		}
	}
	UTIL_Remove( m_pFireHurt );
	m_pFireHurt = NULL;

	// now save as normal
	int status = CBaseMonster::Save( save );
	return status;
}

int CMonsterMolotov::Restore( CRestore &restore )
{
	int status = CBaseMonster::Restore ( restore );
	return status;
}

void CMonsterMolotov::Precache( void )
{
	PRECACHE_MODEL( "models/w_molotov.mdl" );
}

//
// Molotov Explode
//
void CMonsterMolotov::Explode( Vector vecSrc, Vector vecAim )
{
	TraceResult tr;
	UTIL_TraceLine ( pev->origin, pev->origin + Vector ( 0, 0, -32 ),  ignore_monsters, ENT(pev), & tr);

	Explode( &tr, DMG_BLAST );
}

// UNDONE: temporary scorching for PreAlpha - find a less sleazy permenant solution.
void CMonsterMolotov::Explode( TraceResult *pTrace, int bitsDamageType )
{
	float		flRndSound;// sound randomizer

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->flFraction != 1.0 )
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * max(1,(pev->dmg - 24)) * 0.6);
	}

	DROP_TO_FLOOR ( ENT(pev) );

	int iContents = UTIL_PointContents ( pev->origin );

	if (iContents == CONTENTS_WATER)
	{
		UTIL_Remove( this );
		return;
	}

	//MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
	//	WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
	//	WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
	//	WRITE_COORD( pev->origin.y );
	//	WRITE_COORD( pev->origin.z );
	//	WRITE_SHORT( g_sModelIndexFireball );
	//	WRITE_BYTE( (pev->dmg - 50) * .30  ); // scale * 10
	//	WRITE_BYTE( 15  ); // framerate
	//	WRITE_BYTE( TE_EXPLFLAG_NOSOUND );
	//MESSAGE_END();

	/*
	// create a fire sprite
	float dx,dy,dz, SCALE;
	Vector burn;
	burn = pev->origin;
	int x, y, spr;
	for (x = -1; x <= 1; x++)
	{
		for (y = -1; y <= 1; y++)
		{
			spr = 3*(x+1)+(y+1);
			m_pFireSprite[spr] = CSprite::SpriteCreate("sprites/fire.spr",burn,TRUE);
			SCALE = 1.0;
			if (x == 0 && y == 0) SCALE = 2.0;
			m_pFireSprite[spr]->pev->scale = SCALE;
			dx = burn.x + x*m_pFireSprite[spr]->pev->size.x;
			dy = burn.y + y*m_pFireSprite[spr]->pev->size.y;
			dz = burn.z + (0.8 * m_pFireSprite[spr]->pev->size.z * SCALE / 2);
			m_pFireSprite[spr]->pev->origin = Vector(dx,dy,dz);
			m_pFireSprite[spr]->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
			//m_pFireSprite[spr]->pev->spawnflags |= SF_SPRITE_TEMPORARY;
			m_pFireSprite[spr]->pev->framerate = 10.0;
			m_pFireSprite[spr]->TurnOn();
		}
	}

	// create a trigger_hurt entity
	m_pFireHurt = (CTriggerHurt*) CTriggerHurt::Create("trigger_hurt",burn,g_vecZero,edict());
	//m_pFireHurt = (CTriggerHurt*) CTriggerHurt::Create("trigger_hurt",burn,pev->angles,edict());
	Vector v_size = m_pFireSprite[4]->pev->size * 4.0;
	v_size.z = m_pFireSprite[4]->pev->size.z;
	m_pFireHurt->pev->size = v_size;
	m_pFireHurt->pev->absmin = burn - Vector(v_size.x/2,v_size.y/2,0);
	m_pFireHurt->pev->absmax = burn + Vector(v_size.x/2,v_size.y/2,v_size.z);
	m_pFireHurt->pev->dmg = pev->dmg;
	FBitSet(m_pFireHurt->pev->spawnflags,SF_TRIGGER_ALLOWMONSTERS);
	m_pFireHurt->m_bitsDamageInflict |= DMG_BURN;
	*/
	MakeEffects();

	// create a burning clip area
	// THIS DOES NOT WORK DYNAMICALLY, SINCE THE NODE GRAPH ALREADY EXISTS...

	/*
	m_pBurningArea = (CFuncBurningClip*)CFuncBurningClip::Create("func_burning_clip",burn,pev->angles,edict());
	m_pBurningArea->pev->size = v_size;
	m_pBurningArea->pev->absmin = burn - Vector(v_size.x/2,v_size.y/2,0);
	m_pBurningArea->pev->absmax = burn + Vector(v_size.x/2,v_size.y/2,v_size.z);
	*/
	// the hurt doesn't seem to work with snakes very well. I don't know why...

	float duration = RANDOM_LONG(90,150);

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, SMALL_EXPLOSION_VOLUME, 3.0 );
	CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin, 128, duration );
	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	if ( RANDOM_FLOAT( 0 , 1 ) < 0.5 )
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH1 );
	}
	else
	{
		UTIL_DecalTrace( pTrace, DECAL_SCORCH2 );
	}

	flRndSound = RANDOM_FLOAT( 0 , 1 );

	switch ( RANDOM_LONG( 0, 2 ) )
	{
		case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM);	break;
		case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM);	break;
		case 2:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM);	break;
	}

	pev->effects |= EF_NODRAW;

	SetThink( Burning );
	pev->velocity = g_vecZero;
	pev->dmgtime = gpGlobals->time + duration;
	SetNextThink( 0.5 );
}

void CMonsterMolotov::ExplodeTouch( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT(pev), &tr );

	Explode( &tr, DMG_BLAST );
}

void CMonsterMolotov::MakeEffects( void )
{
	// create a fire sprite
	float dx,dy,dz, SCALE;
	Vector burn;
	burn = pev->origin;
	int x, y, spr;
	if (!m_pFireSprite[4])
	{
		for (x = -1; x <= 1; x++)
		{
			for (y = -1; y <= 1; y++)
			{
				spr = 3*(x+1)+(y+1);
				m_pFireSprite[spr] = CSprite::SpriteCreate("sprites/fire.spr",burn,TRUE);
				SCALE = 1.0;
				if (x == 0 && y == 0) SCALE = 2.0;
				m_pFireSprite[spr]->pev->scale = SCALE;
				dx = burn.x + x*m_pFireSprite[spr]->pev->size.x;
				dy = burn.y + y*m_pFireSprite[spr]->pev->size.y;
				dz = burn.z + (0.8 * m_pFireSprite[spr]->pev->size.z * SCALE / 2);
				m_pFireSprite[spr]->pev->origin = Vector(dx,dy,dz);
				m_pFireSprite[spr]->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
				//m_pFireSprite[spr]->pev->spawnflags |= SF_SPRITE_TEMPORARY;
				m_pFireSprite[spr]->pev->framerate = 10.0;
				m_pFireSprite[spr]->TurnOn();
			}
		}
	}

	// if we have restored, then the trigger hurt area is wrong...
	if (!m_pFireHurt)
	{
		m_pFireHurt = (CTriggerHurt*) CTriggerHurt::Create("trigger_hurt",burn,g_vecZero,edict());
		Vector v_size = m_pFireSprite[4]->pev->size * 4.0;
		v_size.z = m_pFireSprite[4]->pev->size.z;
		m_pFireHurt->pev->size = v_size;
		m_pFireHurt->pev->absmin = burn - Vector(v_size.x/2,v_size.y/2,0);
		m_pFireHurt->pev->absmax = burn + Vector(v_size.x/2,v_size.y/2,v_size.z);
		m_pFireHurt->pev->dmg = pev->dmg;
		FBitSet(m_pFireHurt->pev->spawnflags,SF_TRIGGER_ALLOWMONSTERS);
		m_pFireHurt->m_bitsDamageInflict |= DMG_BURN;

		// we also won't have the sound...
		EMIT_SOUND( ENT(pev), CHAN_BODY, "ambience/burning1.wav", 0.5, ATTN_NORM );	
	}
}

void CMonsterMolotov::Burning( void )
{
	MakeEffects();

	// have we burnt out...
	if (gpGlobals->time >= pev->dmgtime)
	{
		BurnOut();
		return;
	}
	else
	{
		//BOOL bRepel = FALSE;
		// for each monster inside the list
		int iMonsters = UTIL_MonstersInSphere(pEntInSphere, MAX_MONSTER, pev->origin, 128);
		for (int i = 0; i < iMonsters; i++)
		{
			if (!(pEntInSphere[i]->pev->flags & FL_MONSTER)) continue;
			if (pEntInSphere[i]->pev->flags & FL_IMMUNE_LAVA) continue;
			if (!(pEntInSphere[i]->IsAlive())) continue;

			// this causes it to listen and evaluate...
			//MonsterThink();
			((CBaseMonster*)pEntInSphere[i])->Listen();
			if (((CBaseMonster*)pEntInSphere[i])->m_iAudibleList != SOUNDLIST_EMPTY)
			{
				((CBaseMonster*)pEntInSphere[i])->ChangeSchedule( GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND ) );
			}


			//bRepel |= Repel((CBaseMonster*)pEntInSphere[i]);
		}

		//if (bRepel)
		//{
			SetNextThink( 0.5 );
		//}
		//else
		//{
		//	SetNextThink( 0.5 );
		//}
	}
}

BOOL CMonsterMolotov::Repel( CBaseMonster* pEnt )
{
	entvars_t* pevEnt = pEnt->pev;

	if ( !FBitSet ( pevEnt->flags , FL_MONSTER ) ) 
	{// touched by a non-monster.
		return FALSE;
	}
	if ( FBitSet ( pevEnt->flags , FL_IMMUNE_LAVA ) ) 
	{// immune to fire.
		return FALSE;
	}
	if ( !pEnt->IsAlive() ) 
	{// don't repel dead things.
		return FALSE;
	}

	pevEnt->origin.z += 1;

	if ( FBitSet ( pevEnt->flags, FL_ONGROUND ) ) 
	{// clear the onground so physics don't bitch
		pevEnt->flags &= ~FL_ONGROUND;
	}

	// calculate the opposite direction, and push in that direction
	Vector vecDirToEnemy = ( pevEnt->origin - pev->origin );
	float distance = vecDirToEnemy.Length2D();
	vecDirToEnemy.z = 0;
	vecDirToEnemy = vecDirToEnemy.Normalize();

	// This is a crap way of doing it, but it is the only way that sort of works for now...
	float speed;
	if (distance < 32) speed = 412;
	else if (distance < 64) speed = 312;
	else if (distance < 92) speed = 256;
	else if (distance < 128) speed = 214;
	else if (distance < 192) speed = 172;
	else if (distance < 256) speed = 92;
	else speed = 64;

	pevEnt->velocity = vecDirToEnemy * speed;
	pevEnt->velocity.z += 16;

	return TRUE;
}

void CMonsterMolotov::BurnOut( void )
{
	int spr;
	STOP_SOUND( ENT(pev), CHAN_BODY, "ambience/burning1.wav" );	
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			spr = 3*(x+1)+(y+1);
			UTIL_Remove( m_pFireSprite[spr] );
			m_pFireSprite[spr] = NULL;
		}
	}
	UTIL_Remove( m_pFireHurt );
	m_pFireHurt = NULL;
	UTIL_Remove( this );
}

void CMonsterMolotov::Killed( entvars_t *pevAttacker, int iGib )
{
	Detonate( );
}


// Timed molotov, this think is called when time runs out.
void CMonsterMolotov::DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( Detonate );
	SetNextThink(0);
}

void CMonsterMolotov::PreDetonate( void )
{
	CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin, 400, 0.3 );

	SetThink( Detonate );
	SetNextThink( 1 );
}


void CMonsterMolotov::Detonate( void )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -40 ),  ignore_monsters, ENT(pev), & tr);

	Explode( &tr, DMG_BLAST );
}

void CMonsterMolotov :: TumbleThink( void )
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	StudioFrameAdvance( );
	SetNextThink( 0.1 );

	if (pev->dmgtime - 1 < gpGlobals->time)
	{
		CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400, 0.1 );
	}

	if (pev->dmgtime <= gpGlobals->time)
	{
		SetThink( Detonate );
	}
	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2;
	}
}


void CMonsterMolotov:: Spawn( void )
{
	Precache( );

	pev->movetype = MOVETYPE_BOUNCE;
	pev->classname = MAKE_STRING( "molotov" );
	
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_molotov.mdl");
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));

	int spr;
	for (int x = -1; x <= 1; x++)
	{
		for (int y = -1; y <= 1; y++)
		{
			spr = 3*(x+1)+(y+1);
			m_pFireSprite[spr] = NULL;
		}
	}
	m_pFireHurt = NULL;
}

void CMonsterMolotov::DangerSoundThink( void )
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5, pev->velocity.Length( ), 0.2 );
	SetNextThink( 0.2 );

	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
	}
}

CMonsterMolotov * CMonsterMolotov::ShootContact( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CMonsterMolotov *pMolotov = GetClassPtr( (CMonsterMolotov *)NULL );
	pMolotov->Spawn();
	// molotovs arc lower
	pMolotov->pev->gravity = 0.5;// lower gravity since molotov is aerodynamic and engine doesn't know it.
	UTIL_SetOrigin( pMolotov, vecStart );
	pMolotov->pev->velocity = vecVelocity;
	pMolotov->pev->angles = UTIL_VecToAngles (pMolotov->pev->velocity);
	pMolotov->pev->owner = ENT(pevOwner);
	
	// make monsters afaid of it while in the air
	pMolotov->SetThink( DangerSoundThink );
	pMolotov->SetNextThink(0);
	
	// Tumble in air
	pMolotov->pev->avelocity.x = RANDOM_FLOAT ( -100, -500 );
	
	// Explode on contact
	pMolotov->SetTouch( ExplodeTouch );

	pMolotov->pev->dmg = gSkillData.plrDmgMolotov;

	pMolotov->pev->spawnflags |= 1;

	return pMolotov;
}


//======================end molotov

