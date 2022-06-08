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
/*

===== combat.cpp ========================================================

  functions dealing with damage infliction & death

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "decals.h"
#include "animation.h"
#include "weapons.h"
#include "func_break.h"
#include "player.h"
#include "game.h"

extern DLL_GLOBAL Vector		g_vecAttackDir;
extern DLL_GLOBAL int			g_iSkillLevel;

extern Vector VecBModelOrigin( entvars_t *pevBModel );
extern entvars_t *g_pevLastInflictor;

unsigned short g_sGibbed;
unsigned short g_sTeleport;
unsigned short g_sTrail;
unsigned short g_sExplosion;
unsigned short g_usPowerUp;

#define GERMAN_GIB_COUNT		4
#define	HUMAN_GIB_COUNT			6
#define ALIEN_GIB_COUNT			4


// HACKHACK -- The gib velocity equations don't work
void CGib::LimitVelocity( void )
{
	float length = pev->velocity.Length();

	// ceiling at 1500.  The gib velocity equation is not bounded properly.  Rather than tune it
	// in 3 separate places again, I'll just limit it here.
	if( length > 1500.0f )
		pev->velocity = pev->velocity.Normalize() * 1500.0f;		// This should really be sv_maxvelocity * 0.75 or something
}


void CGib::SpawnStickyGibs( entvars_t *pevVictim, Vector vecOrigin, int cGibs )
{
	int i;

	if( g_Language == LANGUAGE_GERMAN )
	{
		// no sticky gibs in germany right now!
		return; 
	}

	for( i = 0; i < cGibs; i++ )
	{
		CGib *pGib = GetClassPtr( (CGib *)NULL );

		pGib->Spawn( "models/stickygib.mdl" );
		pGib->pev->body = RANDOM_LONG( 0, 2 );

		if( pevVictim )
		{
			pGib->pev->origin.x = vecOrigin.x + RANDOM_FLOAT( -3.0f, 3.0f );
			pGib->pev->origin.y = vecOrigin.y + RANDOM_FLOAT( -3.0f, 3.0f );
			pGib->pev->origin.z = vecOrigin.z + RANDOM_FLOAT( -3.0f, 3.0f );

			/*
			pGib->pev->origin.x = pevVictim->absmin.x + pevVictim->size.x * ( RANDOM_FLOAT( 0, 1 ) );
			pGib->pev->origin.y = pevVictim->absmin.y + pevVictim->size.y * ( RANDOM_FLOAT( 0, 1 ) );
			pGib->pev->origin.z = pevVictim->absmin.z + pevVictim->size.z * ( RANDOM_FLOAT( 0, 1 ) );
			*/

			// make the gib fly away from the attack vector
			pGib->pev->velocity = g_vecAttackDir * -1.0f;

			// mix in some noise
			pGib->pev->velocity.x += RANDOM_FLOAT( -0.15f, 0.15f );
			pGib->pev->velocity.y += RANDOM_FLOAT( -0.15f, 0.15f );
			pGib->pev->velocity.z += RANDOM_FLOAT( -0.15f, 0.15f );

			pGib->pev->velocity = pGib->pev->velocity * 900.0f;

			pGib->pev->avelocity.x = RANDOM_FLOAT( 250.0f, 400.0f );
			pGib->pev->avelocity.y = RANDOM_FLOAT( 250.0f, 400.0f );

			// copy owner's blood color
			pGib->m_bloodColor = ( CBaseEntity::Instance( pevVictim ) )->BloodColor();

			if( pevVictim->health > -50 )
			{
				pGib->pev->velocity = pGib->pev->velocity * 0.7f;
			}
			else if( pevVictim->health > -200 )
			{
				pGib->pev->velocity = pGib->pev->velocity * 2.0f;
			}
			else
			{
				pGib->pev->velocity = pGib->pev->velocity * 4.0f;
			}

			pGib->pev->movetype = MOVETYPE_TOSS;
			pGib->pev->solid = SOLID_BBOX;
			UTIL_SetSize( pGib->pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
			pGib->SetTouch( &CGib::StickyGibTouch );
			pGib->SetThink( NULL );
		}
		pGib->LimitVelocity();
	}
}

void CGib::SpawnHeadGib( entvars_t *pevVictim )
{
	CGib *pGib = GetClassPtr( (CGib *)NULL );

	if( g_Language == LANGUAGE_GERMAN )
	{
		pGib->Spawn( "models/germangibs.mdl" );// throw one head
		pGib->pev->body = 0;
	}
	else
	{
		pGib->Spawn( "models/hgibs.mdl" );// throw one head
		pGib->pev->body = 0;
	}

	if( pevVictim )
	{
		pGib->pev->origin = pevVictim->origin + pevVictim->view_ofs;

		edict_t *pentPlayer = FIND_CLIENT_IN_PVS( pGib->edict() );

		if( RANDOM_LONG( 0, 100 ) <= 5 && pentPlayer )
		{
			// 5% chance head will be thrown at player's face.
			entvars_t *pevPlayer;

			pevPlayer = VARS( pentPlayer );
			pGib->pev->velocity = ( ( pevPlayer->origin + pevPlayer->view_ofs ) - pGib->pev->origin ).Normalize() * 300.0f;
			pGib->pev->velocity.z += 100.0f;
		}
		else
		{
			pGib->pev->velocity = Vector( RANDOM_FLOAT( -100.0f, 100.0f ), RANDOM_FLOAT( -100.0f, 100.0f ), RANDOM_FLOAT( 200.0f, 300.0f ) );
		}

		pGib->pev->avelocity.x = RANDOM_FLOAT( 100.0f, 200.0f );
		pGib->pev->avelocity.y = RANDOM_FLOAT( 100.0f, 300.0f );

		// copy owner's blood color
		pGib->m_bloodColor = ( CBaseEntity::Instance( pevVictim ) )->BloodColor();

		if( pevVictim->health > -50 )
		{
			pGib->pev->velocity = pGib->pev->velocity * 0.7f;
		}
		else if( pevVictim->health > -200 )
		{
			pGib->pev->velocity = pGib->pev->velocity * 2.0f;
		}
		else
		{
			pGib->pev->velocity = pGib->pev->velocity * 4.0f;
		}
	}
	pGib->LimitVelocity();
}

void CGib::SpawnRandomGibs( entvars_t *pevVictim, int cGibs, int human )
{
	int cSplat;

	for( cSplat = 0; cSplat < cGibs; cSplat++ )
	{
		CGib *pGib = GetClassPtr( (CGib *)NULL );

		if( g_Language == LANGUAGE_GERMAN )
		{
			pGib->Spawn( "models/germangibs.mdl" );
			pGib->pev->body = RANDOM_LONG( 0, GERMAN_GIB_COUNT - 1 );
		}
		else
		{
			if( human )
			{
				// human pieces
				pGib->Spawn( "models/hgibs.mdl" );
				pGib->pev->body = RANDOM_LONG( 1, HUMAN_GIB_COUNT - 1 );// start at one to avoid throwing random amounts of skulls (0th gib)
			}
			else
			{
				// aliens
				pGib->Spawn( "models/agibs.mdl" );
				pGib->pev->body = RANDOM_LONG( 0, ALIEN_GIB_COUNT - 1 );
			}
		}

		if( pevVictim )
		{
			// spawn the gib somewhere in the monster's bounding volume
			pGib->pev->origin.x = pevVictim->absmin.x + pevVictim->size.x * ( RANDOM_FLOAT( 0.0f, 1.0f ) );
			pGib->pev->origin.y = pevVictim->absmin.y + pevVictim->size.y * ( RANDOM_FLOAT( 0.0f, 1.0f ) );
			pGib->pev->origin.z = pevVictim->absmin.z + pevVictim->size.z * ( RANDOM_FLOAT( 0.0f, 1.0f ) ) + 1.0f;	// absmin.z is in the floor because the engine subtracts 1 to enlarge the box

			// make the gib fly away from the attack vector
			pGib->pev->velocity = g_vecAttackDir * -1.0f;

			// mix in some noise
			pGib->pev->velocity.x += RANDOM_FLOAT( -0.25f, 0.25f );
			pGib->pev->velocity.y += RANDOM_FLOAT( -0.25f, 0.25f );
			pGib->pev->velocity.z += RANDOM_FLOAT( -0.25f, 0.25f );

			pGib->pev->velocity = pGib->pev->velocity * RANDOM_FLOAT( 300.0f, 400.0f );

			pGib->pev->avelocity.x = RANDOM_FLOAT( 100.0f, 200.0f );
			pGib->pev->avelocity.y = RANDOM_FLOAT( 100.0f, 300.0f );

			// copy owner's blood color
			pGib->m_bloodColor = ( CBaseEntity::Instance( pevVictim ) )->BloodColor();

			if( pevVictim->health > -50 )
			{
				pGib->pev->velocity = pGib->pev->velocity * 0.7f;
			}
			else if( pevVictim->health > -200 )
			{
				pGib->pev->velocity = pGib->pev->velocity * 2.0f;
			}
			else
			{
				pGib->pev->velocity = pGib->pev->velocity * 4.0f;
			}

			pGib->pev->solid = SOLID_BBOX;
			UTIL_SetSize( pGib->pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
		}
		pGib->LimitVelocity();
	}
}

//=========================================================
// GibMonster - create some gore and get rid of a monster's
// model.
//=========================================================
void CBaseMonster::GibMonster( void )
{
	TraceResult	tr;
	BOOL		gibbed = FALSE;

	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM );

	if( CVAR_GET_FLOAT( "violence_hgibs" ) != 0 )	// Only the player will ever get here
	{
		if( IsPlayer() )
			PLAYBACK_EVENT_FULL ( FEV_GLOBAL, edict(), g_sGibbed, 0.0, pev->origin, g_vecAttackDir, 0.0, 0.0, 0, 0, 0, 0 );

		/*CGib::SpawnHeadGib( pev );
		CGib::SpawnRandomGibs( pev, 4, 1 );// throw some human gibs.*/
	}
	gibbed = TRUE;
}

//=========================================================
// GetDeathActivity - determines the best type of death
// anim to play.
//=========================================================
Activity CBaseMonster::GetDeathActivity( void )
{
	Activity	deathActivity;
	BOOL		fTriedDirection;
	float		flDot;
	TraceResult	tr;
	Vector		vecSrc;

	if( pev->deadflag != DEAD_NO )
	{
		// don't run this while dying.
		return m_IdealActivity;
	}

	vecSrc = Center();

	fTriedDirection = FALSE;
	deathActivity = ACT_DIESIMPLE;// in case we can't find any special deaths to do.

	UTIL_MakeVectors( pev->angles );
	flDot = DotProduct( gpGlobals->v_forward, g_vecAttackDir * -1.0f );

	switch( m_LastHitGroup )
	{
		// try to pick a region-specific death.
	case HITGROUP_HEAD:
		deathActivity = ACT_DIE_HEADSHOT;
		break;
	case HITGROUP_STOMACH:
		deathActivity = ACT_DIE_GUTSHOT;
		break;
	case HITGROUP_GENERIC:
		// try to pick a death based on attack direction
		fTriedDirection = TRUE;
		if( flDot > 0.3f )
		{
			deathActivity = ACT_DIEFORWARD;
		}
		else if( flDot <= -0.3f )
		{
			deathActivity = ACT_DIEBACKWARD;
		}
		break;
	default:
		// try to pick a death based on attack direction
		fTriedDirection = TRUE;

		if( flDot > 0.3f )
		{
			deathActivity = ACT_DIEFORWARD;
		}
		else if( flDot <= -0.3f )
		{
			deathActivity = ACT_DIEBACKWARD;
		}
		break;
	}

	// can we perform the prescribed death?
	if( LookupActivity( deathActivity ) == ACTIVITY_NOT_AVAILABLE )
	{
		// no! did we fail to perform a directional death? 
		if( fTriedDirection )
		{
			// if yes, we're out of options. Go simple.
			deathActivity = ACT_DIESIMPLE;
		}
		else
		{
			// cannot perform the ideal region-specific death, so try a direction.
			if( flDot > 0.3f )
			{
				deathActivity = ACT_DIEFORWARD;
			}
			else if( flDot <= -0.3f )
			{
				deathActivity = ACT_DIEBACKWARD;
			}
		}
	}

	if( LookupActivity( deathActivity ) == ACTIVITY_NOT_AVAILABLE )
	{
		// if we're still invalid, simple is our only option.
		deathActivity = ACT_DIESIMPLE;
	}

	if( deathActivity == ACT_DIEFORWARD )
	{
		// make sure there's room to fall forward
		UTIL_TraceHull( vecSrc, vecSrc + gpGlobals->v_forward * 64.0f, dont_ignore_monsters, head_hull, edict(), &tr );

		if( tr.flFraction != 1.0f )
		{
			deathActivity = ACT_DIESIMPLE;
		}
	}

	if( deathActivity == ACT_DIEBACKWARD )
	{
		// make sure there's room to fall backward
		UTIL_TraceHull( vecSrc, vecSrc - gpGlobals->v_forward * 64.0f, dont_ignore_monsters, head_hull, edict(), &tr );

		if( tr.flFraction != 1.0f )
		{
			deathActivity = ACT_DIESIMPLE;
		}
	}

	return deathActivity;
}

//=========================================================
// GetSmallFlinchActivity - determines the best type of flinch
// anim to play.
//=========================================================
Activity CBaseMonster::GetSmallFlinchActivity( void )
{
	Activity	flinchActivity;
	// BOOL		fTriedDirection;
	//float		flDot;

	// fTriedDirection = FALSE;
	UTIL_MakeVectors( pev->angles );
	//flDot = DotProduct( gpGlobals->v_forward, g_vecAttackDir * -1.0f );

	switch( m_LastHitGroup )
	{
		// pick a region-specific flinch
	case HITGROUP_HEAD:
		flinchActivity = ACT_FLINCH_HEAD;
		break;
	case HITGROUP_STOMACH:
		flinchActivity = ACT_FLINCH_STOMACH;
		break;
	case HITGROUP_LEFTARM:
		flinchActivity = ACT_FLINCH_LEFTARM;
		break;
	case HITGROUP_RIGHTARM:
		flinchActivity = ACT_FLINCH_RIGHTARM;
		break;
	case HITGROUP_LEFTLEG:
		flinchActivity = ACT_FLINCH_LEFTLEG;
		break;
	case HITGROUP_RIGHTLEG:
		flinchActivity = ACT_FLINCH_RIGHTLEG;
		break;
	case HITGROUP_GENERIC:
	default:
		// just get a generic flinch.
		flinchActivity = ACT_SMALL_FLINCH;
		break;
	}

	// do we have a sequence for the ideal activity?
	if( LookupActivity( flinchActivity ) == ACTIVITY_NOT_AVAILABLE )
	{
		flinchActivity = ACT_SMALL_FLINCH;
	}

	return flinchActivity;
}

BOOL CBaseMonster::ShouldGibMonster( int iGib )
{
	if( ( iGib == GIB_NORMAL && pev->health < GIB_HEALTH_VALUE ) || ( iGib == GIB_ALWAYS ) )
		return TRUE;

	return FALSE;
}

void CBaseMonster::CallGibMonster( void )
{
	BOOL fade = FALSE;

	if( HasHumanGibs() )
	{
		if( CVAR_GET_FLOAT( "violence_hgibs" ) == 0.0f )
			fade = TRUE;
	}
	else if( HasAlienGibs() )
	{
		if( CVAR_GET_FLOAT( "violence_agibs" ) == 0.0f )
			fade = TRUE;
	}

	pev->takedamage = DAMAGE_NO;
	pev->solid = SOLID_NOT;// do something with the body. while monster blows up

	if( fade )
	{
		FadeMonster();
	}
	else
	{
		pev->effects = EF_NODRAW; // make the model invisible.
		GibMonster();
	}

	pev->deadflag = DEAD_DEAD;
	FCheckAITrigger();

	// don't let the status bar glitch for players.with <0 health.
	if( pev->health < -99 )
	{
		pev->health = 0;
	}

	if( ShouldFadeOnDeath() && !fade )
		UTIL_Remove( this );
}

//
// fade out - slowly fades a entity out, then removes it.
//
// DON'T USE ME FOR GIBS AND STUFF IN MULTIPLAYER! 
// SET A FUTURE THINK AND A RENDERMODE!!
void CBaseEntity::SUB_StartFadeOut( void )
{
	if( pev->rendermode == kRenderNormal )
	{
		pev->renderamt = 255;
		pev->rendermode = kRenderTransTexture;
	}

	pev->solid = SOLID_NOT;
	pev->avelocity = g_vecZero;

	pev->nextthink = gpGlobals->time + 0.1f;
	SetThink( &CBaseEntity::SUB_FadeOut );
}

void CBaseEntity::SUB_FadeOut( void )
{
	if( pev->renderamt > 7 )
	{
		pev->renderamt -= 7;
		pev->nextthink = gpGlobals->time + 0.1f;
	}
	else 
	{
		pev->renderamt = 0;
		pev->nextthink = gpGlobals->time + 0.2f;
		SetThink( &CBaseEntity::SUB_Remove );
	}
}

//=========================================================
// WaitTillLand - in order to emit their meaty scent from
// the proper location, gibs should wait until they stop 
// bouncing to emit their scent. That's what this function
// does.
//=========================================================
void CGib::WaitTillLand( void )
{
	if( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	if( pev->velocity == g_vecZero )
	{
		SetThink( &CBaseEntity::SUB_StartFadeOut );
		pev->nextthink = gpGlobals->time + m_lifeTime;
	}
	else
	{
		// wait and check again in another half second.
		pev->nextthink = gpGlobals->time + 0.5f;
	}
}

//
// Gib bounces on the ground or wall, sponges some blood down, too!
//
void CGib::BounceGibTouch( CBaseEntity *pOther )
{
	Vector	vecSpot;
	TraceResult	tr;

	//if( RANDOM_LONG( 0, 1 ) )
	//	return;// don't bleed everytime

	if( pev->flags & FL_ONGROUND )
	{
		pev->velocity = pev->velocity * 0.9f;
		pev->angles.x = 0.0f;
		pev->angles.z = 0.0f;
		pev->avelocity.x = 0.0f;
		pev->avelocity.z = 0.0f;
	}
	else
	{
		if( g_Language != LANGUAGE_GERMAN && m_cBloodDecals > 0 && m_bloodColor != DONT_BLEED )
		{
			vecSpot = pev->origin + Vector( 0.0f, 0.0f, 8.0f );//move up a bit, and trace down.
			UTIL_TraceLine( vecSpot, vecSpot + Vector( 0.0f, 0.0f, -24.0f ), ignore_monsters, ENT( pev ), &tr );

			UTIL_BloodDecalTrace( &tr, m_bloodColor );

			m_cBloodDecals--; 
		}

		if( m_material != matNone && RANDOM_LONG( 0, 2 ) == 0 )
		{
			float volume;
			float zvel = fabs( pev->velocity.z );

			volume = 0.8f * Q_min( 1.0f, zvel / 450.0f );

			CBreakable::MaterialSoundRandom( edict(), (Materials)m_material, volume );
		}
	}
}

//
// Sticky gib puts blood on the wall and stays put. 
//
void CGib::StickyGibTouch( CBaseEntity *pOther )
{
	Vector	vecSpot;
	TraceResult	tr;

	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 10.0f;

	if( !FClassnameIs( pOther->pev, "worldspawn" ) )
	{
		pev->nextthink = gpGlobals->time;
		return;
	}

	UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 32.0f,  ignore_monsters, ENT( pev ), &tr );

	UTIL_BloodDecalTrace( &tr, m_bloodColor );

	pev->velocity = tr.vecPlaneNormal * -1.0f;
	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
}

//
// Throw a chunk
//
void CGib::Spawn( const char *szGibModel )
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->friction = 0.55f; // deading the bounce a bit

	// sometimes an entity inherits the edict from a former piece of glass,
	// and will spawn using the same render FX or rendermode! bad!
	pev->renderamt = 255;
	pev->rendermode = kRenderNormal;
	pev->renderfx = kRenderFxNone;
	pev->solid = SOLID_SLIDEBOX;/// hopefully this will fix the VELOCITY TOO LOW crap
	pev->classname = MAKE_STRING( "gib" );

	SET_MODEL( ENT( pev ), szGibModel );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	pev->nextthink = gpGlobals->time + 4.0f;
	m_lifeTime = 25;
	SetThink( &CGib::WaitTillLand );
	SetTouch( &CGib::BounceGibTouch );

	m_material = matNone;
	m_cBloodDecals = 5;// how many blood decals this gib can place (1 per bounce until none remain). 
}

/*
============
TakeDamage

The damage is coming from inflictor, but get mad at attacker
This should be the only function that ever reduces health.
bitsDamageType indicates the type of damage sustained, ie: DMG_SHOCK

Time-based damage: only occurs while the monster is within the trigger_hurt.
When a monster is poisoned via an arrow etc it takes all the poison damage at once.

GLOBALS ASSUMED SET:  g_iSkillLevel
============
*/
int CBaseMonster::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	float	flTake;
	Vector	vecDir;

	if( !pev->takedamage )
		return 0;

	if( !IsAlive() )
	{
		return DeadTakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
	}

	if( pev->deadflag == DEAD_NO )
	{
		// no pain sound during death animation.
		PainSound();// "Ouch!"
	}

	//!!!LATER - make armor consideration here!
	flTake = flDamage;

	// set damage type sustained
	m_bitsDamageType |= bitsDamageType;

	// grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	vecDir = Vector( 0, 0, 0 );
	if( !FNullEnt( pevInflictor ) )
	{
		CBaseEntity *pInflictor = CBaseEntity::Instance( pevInflictor );
		if( pInflictor )
		{
			vecDir = ( pInflictor->Center() - Vector ( 0, 0, 10 ) - Center() ).Normalize();
			vecDir = g_vecAttackDir = vecDir.Normalize();
		}
	}

	// add to the damage total for clients, which will be sent as a single
	// message at the end of the frame
	// todo: remove after combining shotgun blasts?
	if( IsPlayer() )
	{
		if( pevInflictor )
			pev->dmg_inflictor = ENT( pevInflictor );

		pev->dmg_take += flTake;

		// check for godmode or invincibility
		if( pev->flags & FL_GODMODE )
		{
			return 0;
		}
	}

	// if this is a player, move him around!
	if( ( !FNullEnt( pevInflictor ) ) && ( pev->movetype == MOVETYPE_WALK ) && ( !pevAttacker || pevAttacker->solid != SOLID_TRIGGER ) )
	{
		pev->velocity = pev->velocity + vecDir * -DamageForce( flDamage );
	}

	// do the damage
	pev->health -= flTake;

	// HACKHACK Don't kill monsters in a script.  Let them break their scripts first
	if( m_MonsterState == MONSTERSTATE_SCRIPT )
	{
		SetConditions( bits_COND_LIGHT_DAMAGE );
		return 0;
	}

	if( pev->health <= 0 )
	{
		g_pevLastInflictor = pevInflictor;

		if( bitsDamageType & DMG_ALWAYSGIB )
		{
			Killed( pevAttacker, GIB_ALWAYS );
		}
		else if( bitsDamageType & DMG_NEVERGIB )
		{
			Killed( pevAttacker, GIB_NEVER );
		}
		else
		{
			Killed( pevAttacker, GIB_NORMAL );
		}

		g_pevLastInflictor = NULL;

		return 0;
	}

	// react to the damage (get mad)
	if( ( pev->flags & FL_MONSTER ) && !FNullEnt( pevAttacker ) )
	{
		if( pevAttacker->flags & ( FL_MONSTER | FL_CLIENT ) )
		{
			// only if the attack was a monster or client!
			// enemy's last known position is somewhere down the vector that the attack came from.
			if( pevInflictor )
			{
				if( m_hEnemy == 0 || pevInflictor == m_hEnemy->pev || !HasConditions( bits_COND_SEE_ENEMY ) )
				{
					m_vecEnemyLKP = pevInflictor->origin;
				}
			}
			else
			{
				m_vecEnemyLKP = pev->origin + ( g_vecAttackDir * 64.0f );
			}

			MakeIdealYaw( m_vecEnemyLKP );

			// add pain to the conditions 
			// !!!HACKHACK - fudged for now. Do we want to have a virtual function to determine what is light and 
			// heavy damage per monster class?
			if( flDamage > 0.0f )
			{
				SetConditions( bits_COND_LIGHT_DAMAGE );
			}

			if( flDamage >= 20.0f )
			{
				SetConditions( bits_COND_HEAVY_DAMAGE );
			}
		}
	}

	return 1;
}

//=========================================================
// DeadTakeDamage - takedamage function called when a monster's
// corpse is damaged.
//=========================================================
int CBaseMonster::DeadTakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Vector vecDir;

	// grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	vecDir = Vector( 0, 0, 0 );
	if( !FNullEnt( pevInflictor ) )
	{
		CBaseEntity *pInflictor = CBaseEntity::Instance( pevInflictor );
		if( pInflictor )
		{
			vecDir = ( pInflictor->Center() - Vector ( 0.0f, 0.0f, 10.0f ) - Center() ).Normalize();
			vecDir = g_vecAttackDir = vecDir.Normalize();
		}
	}

#if 0// turn this back on when the bounding box issues are resolved.

	pev->flags &= ~FL_ONGROUND;
	pev->origin.z += 1.0f;

	// let the damage scoot the corpse around a bit.
	if( !FNullEnt( pevInflictor ) && ( pevAttacker->solid != SOLID_TRIGGER ) )
	{
		pev->velocity = pev->velocity + vecDir * -DamageForce( flDamage );
	}
#endif
	// kill the corpse if enough damage was done to destroy the corpse and the damage is of a type that is allowed to destroy the corpse.
	if( bitsDamageType & DMG_GIB_CORPSE )
	{
		if( pev->health <= flDamage )
		{
			pev->health = -50;
			Killed( pevAttacker, GIB_ALWAYS );
			return 0;
		}
		// Accumulate corpse gibbing damage, so you can gib with multiple hits
		pev->health -= flDamage * 0.1f;
	}

	return 1;
}

float CBaseMonster::DamageForce( float damage )
{ 
	float force = damage * ( ( 32.0f * 32.0f * 72.0f ) / ( pev->size.x * pev->size.y * pev->size.z ) ) * 5.0f;

	if( force > 1000.0f ) 
	{
		force = 1000.0f;
	}

	return force;
}

//
// RadiusDamage - this entity is exploding, or otherwise needs to inflict damage upon entities within a certain range.
// 
// only damage ents that can clearly be seen by the explosion!
void RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType )
{
	CBaseEntity *pEntity = NULL;
	TraceResult	tr;
	float		flAdjustedDamage, falloff;
	Vector		vecSpot;

	if( flRadius )
		falloff = flDamage / flRadius;
	else
		falloff = 1.0f;

	int bInWater = ( UTIL_PointContents( vecSrc ) == CONTENTS_WATER );

	vecSrc.z += 1.0f;// in case grenade is lying on the ground

	if( !pevAttacker )
		pevAttacker = pevInflictor;

	// iterate on all entities in the vicinity.
	while( ( pEntity = UTIL_FindEntityInSphere( pEntity, vecSrc, flRadius ) ) != NULL )
	{
		if( pEntity->pev->takedamage != DAMAGE_NO )
		{
			// UNDONE: this should check a damage mask, not an ignore
			if( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
			{
				// houndeyes don't hurt other houndeyes with their attack
				continue;
			}

			// blast's don't tavel into or out of water
			if( bInWater && pEntity->pev->waterlevel == 0 )
				continue;
			if( !bInWater && pEntity->pev->waterlevel == 3 )
				continue;

			vecSpot = pEntity->BodyTarget( vecSrc );

			UTIL_TraceLine( vecSrc, vecSpot, dont_ignore_monsters, ENT( pevInflictor ), &tr );

			if( tr.flFraction == 1.0f || tr.pHit == pEntity->edict() )
			{
				// the explosion can 'see' this entity, so hurt them!
				if( tr.fStartSolid )
				{
					// if we're stuck inside them, fixup the position and distance
					tr.vecEndPos = vecSrc;
					tr.flFraction = 0.0f;
				}

				// decrease damage for an ent that's farther from the bomb.
				flAdjustedDamage = ( vecSrc - tr.vecEndPos ).Length() * falloff;
				flAdjustedDamage = flDamage - flAdjustedDamage;

				if( flAdjustedDamage < 0.0f )
				{
					flAdjustedDamage = 0.0f;
				}

				// ALERT( at_console, "hit %s\n", STRING( pEntity->pev->classname ) );
				if( tr.flFraction != 1.0f )
				{
					ClearMultiDamage();
					pEntity->TraceAttack( pevInflictor, flAdjustedDamage, ( tr.vecEndPos - vecSrc ).Normalize(), &tr, bitsDamageType );
					ApplyMultiDamage( pevInflictor, pevAttacker );
				}
				else
				{
					pEntity->TakeDamage ( pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType );
				}
			}
		}
	}
}

void CBaseMonster::RadiusDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType )
{
	::RadiusDamage( pev->origin, pevInflictor, pevAttacker, flDamage, flDamage * 2.5f, iClassIgnore, bitsDamageType );
}

void CBaseMonster::RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType )
{
	::RadiusDamage( vecSrc, pevInflictor, pevAttacker, flDamage, flDamage * 2.5f, iClassIgnore, bitsDamageType );
}

/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.

This version is used by Players, uses the random seed generator to sync client and server side shots.
================
*/
Vector CBaseEntity::FireBulletsPlayer( ULONG cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq, int iDamage, entvars_t *pevAttacker, int shared_rand )
{
	static int tracerCount;
	TraceResult tr;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;
	float x = 0.0f, y = 0.0f;
	//float z;

	if( pevAttacker == NULL )
		pevAttacker = pev;  // the default attacker is ourselves

	ClearMultiDamage();
	gMultiDamage.type = DMG_BULLET | DMG_NEVERGIB;

	for( ULONG iShot = 1; iShot <= cShots; iShot++ )
	{
		//Use player's random seed.
		// get circular gaussian spread
		x = UTIL_SharedRandomFloat( shared_rand + iShot, -0.5f, 0.5f ) + UTIL_SharedRandomFloat( shared_rand + ( 1 + iShot ) , -0.5f, 0.5f );
		y = UTIL_SharedRandomFloat( shared_rand + ( 2 + iShot ), -0.5f, 0.5f ) + UTIL_SharedRandomFloat( shared_rand + ( 3 + iShot ), -0.5f, 0.5f );
		//z = x * x + y * y;

		Vector vecDir = vecDirShooting +
						x * vecSpread.x * vecRight +
						y * vecSpread.y * vecUp;
		Vector vecEnd;

		vecEnd = vecSrc + vecDir * flDistance;
		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev )/*pentIgnore*/, &tr );

		// do damage, paint decals
		if( tr.flFraction != 1.0f )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

			if( iDamage )
			{
				pEntity->TraceAttack( pevAttacker, iDamage, vecDir, &tr, DMG_BULLET | ( ( iDamage > 16 ) ? DMG_ALWAYSGIB : DMG_NEVERGIB ) );

				TEXTURETYPE_PlaySound( &tr, vecSrc, vecEnd, iBulletType );
				DecalGunshot( &tr, iBulletType );
			} 
			else switch( iBulletType )
			{
			default:
			case BULLET_PLAYER_9MM:
				pEntity->TraceAttack( pevAttacker, gSkillData.plrDmg9MM, vecDir, &tr, DMG_BULLET );
				break;
			case BULLET_PLAYER_MP5:
				pEntity->TraceAttack( pevAttacker, gSkillData.plrDmgMP5, vecDir, &tr, DMG_BULLET );
				break;
			case BULLET_PLAYER_BUCKSHOT:
				 // make distance based!
				pEntity->TraceAttack( pevAttacker, gSkillData.plrDmgBuckshot, vecDir, &tr, DMG_BULLET );
				break;
			case BULLET_PLAYER_357:
				pEntity->TraceAttack( pevAttacker, gSkillData.plrDmg357, vecDir, &tr, DMG_BULLET );
				break;
			case BULLET_NONE: // FIX
				pEntity->TraceAttack( pevAttacker, 50, vecDir, &tr, DMG_CLUB );
				TEXTURETYPE_PlaySound( &tr, vecSrc, vecEnd, iBulletType );
				// only decal glass
				if( !FNullEnt( tr.pHit ) && VARS( tr.pHit )->rendermode != 0 )
				{
					UTIL_DecalTrace( &tr, DECAL_GLASSBREAK1 + RANDOM_LONG( 0, 2 ) );
				}

				break;
			}
		}
		// make bullet trails
		UTIL_BubbleTrail( vecSrc, tr.vecEndPos, (int)( ( flDistance * tr.flFraction ) / 64.0f ) );
	}
	ApplyMultiDamage( pev, pevAttacker );

	return Vector( x * vecSpread.x, y * vecSpread.y, 0.0 );
}

void CBaseEntity::TraceBleed( float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if( BloodColor() == DONT_BLEED )
		return;

	if( flDamage == 0 )
		return;

	if( !( bitsDamageType & ( DMG_CRUSH | DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB | DMG_MORTAR ) ) )
		return;

	// make blood decal on the wall! 
	TraceResult Bloodtr;
	Vector vecTraceDir; 
	float flNoise;
	int cCount;
	int i;

/*
	if( !IsAlive() )
	{
		// dealing with a dead monster. 
		if( pev->max_health <= 0 )
		{
			// no blood decal for a monster that has already decalled its limit.
			return; 
		}
		else
		{
			pev->max_health--;
		}
	}
*/
	if( flDamage < 10.0f )
	{
		flNoise = 0.1f;
		cCount = 1;
	}
	else if( flDamage < 25.0f )
	{
		flNoise = 0.2f;
		cCount = 2;
	}
	else
	{
		flNoise = 0.3f;
		cCount = 4;
	}

	for( i = 0; i < cCount; i++ )
	{
		vecTraceDir = vecDir * -1.0f;// trace in the opposite direction the shot came from (the direction the shot is going)

		vecTraceDir.x += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.y += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.z += RANDOM_FLOAT( -flNoise, flNoise );

		UTIL_TraceLine( ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * -172.0f, ignore_monsters, ENT( pev ), &Bloodtr );

		if( Bloodtr.flFraction != 1.0f )
		{
			UTIL_BloodDecalTrace( &Bloodtr, BloodColor() );
		}
	}
}

//=========================================================
//=========================================================
void CBaseMonster::MakeDamageBloodDecal( int cCount, float flNoise, TraceResult *ptr, const Vector &vecDir )
{
	// make blood decal on the wall! 
	TraceResult Bloodtr;
	Vector vecTraceDir; 
	int i;

	if( !IsAlive() )
	{
		// dealing with a dead monster. 
		if( pev->max_health <= 0 )
		{
			// no blood decal for a monster that has already decalled its limit.
			return; 
		}
		else
		{
			pev->max_health--;
		}
	}

	for( i = 0; i < cCount; i++ )
	{
		vecTraceDir = vecDir;

		vecTraceDir.x += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.y += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.z += RANDOM_FLOAT( -flNoise, flNoise );

		UTIL_TraceLine( ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * 172.0f, ignore_monsters, ENT( pev ), &Bloodtr );

/*
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_SHOWLINE);
			WRITE_COORD( ptr->vecEndPos.x );
			WRITE_COORD( ptr->vecEndPos.y );
			WRITE_COORD( ptr->vecEndPos.z );

			WRITE_COORD( Bloodtr.vecEndPos.x );
			WRITE_COORD( Bloodtr.vecEndPos.y );
			WRITE_COORD( Bloodtr.vecEndPos.z );
		MESSAGE_END();
*/

		if( Bloodtr.flFraction != 1.0f )
		{
			UTIL_BloodDecalTrace( &Bloodtr, BloodColor() );
		}
	}
}
