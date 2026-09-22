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
// Hornets
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"gamerules.h"

class Cstukabat : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int	 Classify ( void );
	int  IRelationship ( CBaseEntity *pTarget );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void EXPORT StartTrack ( void );
	void EXPORT TrackTarget ( void );
	void EXPORT TrackTouch ( CBaseEntity *pOther );
	void EXPORT DieTouch ( CBaseEntity *pOther );

	float			m_flStopAttack;
	float			m_flFlySpeed;

	EHANDLE m_hOwner;
};

LINK_ENTITY_TO_CLASS( dragon_strike, Cstukabat );

//=========================================================
// Save/Restore
//=========================================================
TYPEDESCRIPTION	Cstukabat::m_SaveData[] = 
{
	DEFINE_FIELD( Cstukabat, m_flStopAttack, FIELD_TIME ),
	DEFINE_FIELD( Cstukabat, m_flFlySpeed, FIELD_FLOAT ),
	DEFINE_FIELD( Cstukabat, m_hOwner, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( Cstukabat, CBaseMonster );

//=========================================================
//=========================================================
void Cstukabat :: Spawn( void )
{
	Precache();

	pev->movetype	= MOVETYPE_BOUNCEMISSILE;
	pev->solid		= SOLID_BBOX;
	pev->takedamage = DAMAGE_NO;
	pev->flags		|= FL_MONSTER;
	m_bloodColor	= DONT_BLEED;
	m_flFieldOfView = -1; // +- 90 degrees
	pev->renderfx	= 255;
	pev->dmg = 80;
	pev->health = 160;

	SET_MODEL(ENT( pev ), "models/dragon_skill.mdl");

	m_flStopAttack	= gpGlobals->time + 9.0;
	m_flFlySpeed    = (float)600;

	UTIL_SetSize( pev, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ) );

	SetThink( &Cstukabat::StartTrack );

	edict_t *pSoundEnt = pev->owner;
	if ( !pSoundEnt ){
		pSoundEnt = edict();
	}
	
	pev->flags	|= FL_NOTARGET;
	m_lovehate = 100;

	pev->nextthink = gpGlobals->time + 0.1;

	if(pev->owner != NULL){
	m_hOwner = Instance( pev->owner );
	}
}


void Cstukabat :: Precache()
{
	PRECACHE_MODEL("models/dragon_skill.mdl");

	PRECACHE_SOUND( "weapons/stkb_die1.wav" );
	PRECACHE_SOUND( "weapons/stkb_fire1.wav" );
	PRECACHE_SOUND( "weapons/stkb_idletpick1.wav" );
	PRECACHE_SOUND( "weapons/flashbang-1.wav" );
}	

//=========================================================
// hornets will never get mad at each other, no matter who the owner is.
//=========================================================
int Cstukabat::IRelationship ( CBaseEntity *pTarget )
{
	if ( pTarget->pev->modelindex == pev->modelindex )
	{
		return R_NO;
	}

	return CBaseMonster :: IRelationship( pTarget );
}

//=========================================================
// ID's Hornet as their owner
//=========================================================
int Cstukabat::Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
// StartTrack - starts a hornet out tracking its target
//=========================================================
void Cstukabat :: StartTrack ( void )
{
	pev->effects |= EF_DIMLIGHT;

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_BEAMFOLLOW );
	WRITE_SHORT( entindex() );		// entity, attachment
	WRITE_SHORT(g_sModelIndexTrail );	// model
	WRITE_BYTE( 10 ); // life
	WRITE_BYTE( 20 );  // width
	WRITE_BYTE( 255 );	// R
	WRITE_BYTE( 255 );	// G
	WRITE_BYTE( 128 );	// B
	WRITE_BYTE( 192 );	// brightness
	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	SetTouch( &Cstukabat::TrackTouch );
	SetThink( &Cstukabat::TrackTarget );

	pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
// Hornet is flying, gently tracking target
//=========================================================
void Cstukabat :: TrackTarget ( void )
{
	Vector	vecFlightDir;
	Vector	vecDirToEnemy;
	float	flDelta;

	StudioFrameAdvance( );

	if (gpGlobals->time > m_flStopAttack || pev->frags >= 12)
	{
		FX_Explosion( Center(), 44);

		SetTouch( NULL );
		SetThink( &Cstukabat::SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

// UNDONE: The player pointer should come back after returning from another level
	if ( m_hEnemy == NULL )
	{// enemy is dead.
		Look( 4096 );
		m_hEnemy = BestVisibleEnemy( );
	}
	else if(m_hEnemy->pev->deadflag != DEAD_NO){
		Look( 4096 );
		m_hEnemy = BestVisibleEnemy( );
	}

	if ( m_hEnemy != NULL && FVisible( m_hEnemy ))
	{
		int rand = RANDOM_LONG(0,3);
		if (rand == 0){
		m_vecEnemyLKP = m_hEnemy->BodyTarget( pev->origin );
		}
		else if (rand == 3){
		m_vecEnemyLKP = m_hEnemy->BodyTarget_e( pev->origin );
		}
		else{
		m_vecEnemyLKP = m_hEnemy->BodyTarget_o( pev->origin );
		}
	}
	else
	{
		m_vecEnemyLKP = m_vecEnemyLKP + pev->velocity * m_flFlySpeed * 0.1;
	}

	vecDirToEnemy = ( m_vecEnemyLKP - pev->origin ).Normalize();

	if (pev->velocity.Length() < 0.1)
		vecFlightDir = vecDirToEnemy;
	else 
		vecFlightDir = pev->velocity.Normalize();

	// measure how far the turn is, the wider the turn, the slow we'll go this time.
	flDelta = DotProduct ( vecFlightDir, vecDirToEnemy );

	pev->velocity = ( vecFlightDir + vecDirToEnemy).Normalize();
	
	pev->velocity = pev->velocity * m_flFlySpeed;// do not have to slow down to turn.
	pev->nextthink = gpGlobals->time + 0.1;// fixed think time

	pev->angles = UTIL_VecToAngles (pev->velocity);

	pev->solid = SOLID_BBOX;

	// if hornet is close to the enemy, jet in a straight line for a half second.
	// (only in the single player game)
	if ( m_hEnemy != NULL )
	{
		if ( flDelta >= 0.5 && ( pev->origin - m_vecEnemyLKP ).Length() <= 512 )
		{
			pev->velocity = pev->velocity * 2;
			pev->nextthink = gpGlobals->time + 0.3;
		}
	}
}

//=========================================================
// Tracking Hornet hit something
//=========================================================
void Cstukabat :: TrackTouch ( CBaseEntity *pOther )
{
	if ( pOther->edict() == pev->owner || pOther->pev->modelindex == pev->modelindex )
	{// bumped into the guy that shot it.
		pev->solid = SOLID_NOT;
		return;
	}

	
	if ( IRelationship( pOther ) <= R_NO)
	{
		// hit something we don't want to hurt, so turn around.

	//	pev->velocity = pev->velocity.Normalize();

	//	pev->velocity.x *= -1;
	//	pev->velocity.y *= -1;

	//	pev->origin = pev->origin + pev->velocity * 4; // bounce the hornet off a bit.
	//	pev->velocity = pev->velocity * m_flFlySpeed;

		return;
	}
	

	DieTouch( pOther );
}

void Cstukabat::DieTouch ( CBaseEntity *pOther )
{
	if(pev->frags <= 11){
		TraceResult tr = UTIL_GetGlobalTrace( );
		if ( pOther && pOther->pev->takedamage )
		{
			pev->frags += 1;

			UTIL_Sparks(tr.vecEndPos);

			switch (RANDOM_LONG(0,1))
			{// buzz when you plug someone
				case 0:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/bullet_hit1.wav", 1, ATTN_NORM);	break;
				case 1:	EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/bullet_hit2.wav", 1, ATTN_NORM);	break;
			}

			ClearMultiDamage( );
			pOther->TraceAttack(pev, pev->dmg, gpGlobals->v_forward, &tr, DMG_SLASH | DMG_ENERGYBEAM); 
			if(m_hOwner){
			m_hOwner->TakeHealth(16, DMG_GENERIC);//��Ѫ
			}
			ApplyMultiDamage( pev, VARS( pev->owner ) );
		}
	}
}

