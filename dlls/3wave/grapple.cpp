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
/***********************************************
************************************************
				    GRAPPLE
************************************************
***********************************************/

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"grapple.h"

unsigned short g_usHook;
unsigned short g_usCable;

void CGrapple::Reset_Grapple()
{
	CBasePlayer *pOwner = (CBasePlayer *)CBaseEntity::Instance( pev->owner );

	pOwner->m_bOn_Hook = FALSE;
	pOwner->m_bHook_Out = FALSE;

	PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, 
	pOwner->edict(), g_usCable, 0, (float*)&g_vecZero, (float*)&g_vecZero, 
	0.0, 0.0, entindex(), pev->team, 1, 0 );

	STOP_SOUND( edict(), CHAN_WEAPON, "weapons/grhang.wav" );
	STOP_SOUND( pOwner->edict(), CHAN_WEAPON, "weapons/grfire.wav" );
	STOP_SOUND( pOwner->edict(), CHAN_WEAPON, "weapons/grpull.wav" );

	pOwner->m_ppHook = NULL;
	pev->enemy = NULL;

        UTIL_Remove( this );
}

void CGrapple::GrappleTouch( CBaseEntity *pOther )
{
	CBasePlayer *pOwner = (CBasePlayer *)CBaseEntity::Instance( pev->owner );

	if( pOther == pOwner )
		return;

	// DO NOT allow the grapple to hook to any projectiles, no matter WHAT!
	// if you create new types of projectiles, make sure you use one of the
	// classnames below or write code to exclude your new classname so
	// grapples will not stick to them.
	if( pOther->Classify() == CLASS_PROJECTILE )
		return;

	if( pOther->IsPlayer() )
	{
		// glance off of teammates
		if( pOther->pev->team == pOwner->pev->team )
			return;

		// sound( self, CHAN_WEAPON, "player/axhit1.wav", 1, ATTN_NORM );
		// TakeDamage( pOther->pev, pOwner->pev, 10, DMG_GENERIC );

		// make hook invisible since we will be pulling directly
		// towards the player the hook hit. Quakeworld makes it
		// too quirky to try to match hook's velocity with that of
		// the client that it hit.
		// setmodel( self, "");
		pev->velocity = g_vecZero;
		UTIL_SetOrigin( pev, pOther->pev->origin );
	}
	else
	{
		// sound( self, CHAN_WEAPON, "player/axhit2.wav", 1, ATTN_NORM );

                // One point of damage inflicted upon impact. Subsequent
                // damage will only be done to PLAYERS... this way secret
                // doors and triggers will only be damaged once.
                if( pOther->pev->takedamage )
                        TakeDamage( pOther->pev, pOwner->pev, 1, DMG_GENERIC );

                pev->velocity = g_vecZero;
		EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/grhit.wav", 1, ATTN_NORM );

		// No sparks underwater
		if( pev->waterlevel == 0 )
			UTIL_Sparks( pev->origin );
	}

	// conveniently clears the sound channel of the CHAIN1 sound,
	// which is a looping sample and would continue to play. Tink1 is
	// the least offensive choice, ass NULL.WAV loops and clogs the
	// channel with silence
	//  sound (self.owner, CHAN_NO_PHS_ADD+CHAN_WEAPON, "weapons/tink1.wav", 1, ATTN_NORM);

	if( !( pOwner->pev->button & IN_ATTACK ) )
	{
		if( pOwner->m_bOn_Hook ) 
		{
			Reset_Grapple();
			return;
		}
	}

	if( pOwner->pev->flags & FL_ONGROUND )
	{
		pOwner->pev->flags &= ~FL_ONGROUND;
//		setorigin(self.owner,self.owner.origin + '0 0 1');
	}

	pOwner->m_bOn_Hook = TRUE;

	// sound( self.owner, CHAN_WEAPON, "weapons/chain2.wav", 1, ATTN_NORM );

	// CHAIN2 is a looping sample. Use LEFTY as a flag so that client.qc
	// will know to only play the tink sound ONCE to clear the weapons
	// sound channel. (Lefty is a leftover from AI.QC, so I reused it to
	// avoid adding a field)
	//self.owner.lefty = TRUE;
	
	STOP_SOUND( pOwner->edict(), CHAN_WEAPON, "weapons/grfire.wav" );

	pev->enemy = pOther->edict();// remember this guy!
	SetThink( &CGrapple::Grapple_Track );
	pev->nextthink = gpGlobals->time;
	m_flNextIdleTime = gpGlobals->time + 0.1;
	pev->solid = SOLID_NOT;
	SetTouch( NULL );
}

bool CanSee( CBaseEntity *pEnemy, CBaseEntity *pOwner )
{
	TraceResult tr;

	UTIL_TraceLine( pOwner->pev->origin, pEnemy->pev->origin,  ignore_monsters, ENT( pOwner->pev ), &tr );
	if( tr.flFraction == 1 )
		return TRUE;

	UTIL_TraceLine( pOwner->pev->origin, pEnemy->pev->origin + Vector( 15, 15, 0 ), ignore_monsters, ENT( pOwner->pev ), &tr );
	if( tr.flFraction == 1 )
		return TRUE;

	UTIL_TraceLine( pOwner->pev->origin, pEnemy->pev->origin + Vector( -15, -15, 0 ), ignore_monsters, ENT( pOwner->pev ), &tr );
	if( tr.flFraction == 1 )
		return TRUE;

	UTIL_TraceLine( pOwner->pev->origin, pEnemy->pev->origin + Vector( -15, 15, 0 ), ignore_monsters, ENT( pOwner->pev ), &tr );
	if( tr.flFraction == 1 )
		return TRUE;

	UTIL_TraceLine( pOwner->pev->origin, pEnemy->pev->origin + Vector( 15, -15, 0 ), ignore_monsters, ENT( pOwner->pev ), &tr );
	if( tr.flFraction == 1 )
		return TRUE;

	return FALSE;
}

void CGrapple::Grapple_Track()
{
	CBasePlayer *pOwner = (CBasePlayer*)CBaseEntity::Instance( pev->owner );
	CBaseEntity *pEnemy = CBaseEntity::Instance( pev->enemy );

	// Release dead targets
	if( pEnemy->IsPlayer() && pEnemy->pev->health <= 0 )
		Reset_Grapple();

	// drop the hook if owner is dead or has released the button
	if( !pOwner->m_bOn_Hook || pOwner->pev->health <= 0 )
	{
		Reset_Grapple();
		return;
	}

	if( !( pOwner->pev->button & IN_ATTACK ) )
	{
		if( pOwner->m_iQuakeWeapon == IT_EXTRA_WEAPON ) 
		{
			Reset_Grapple();
			return;
		}
	}

	// bring the pAiN!
	if( pEnemy->IsPlayer() )
	{
		if( !CanSee( pEnemy, pOwner ) ) 
		{
			Reset_Grapple();
			return;
		}

		// move the hook along with the player.  It's invisible, but
		// we need this to make the sound come from the right spot
		UTIL_SetOrigin( pev, pEnemy->pev->origin );

		// sound( self, CHAN_WEAPON, "blob/land1.wav", 1, ATTN_NORM );

		SpawnBlood( pEnemy->pev->origin, BLOOD_COLOR_RED, 1 );
		( (CBasePlayer *)pEnemy )->TakeDamage( pev, pOwner->pev, 1, DMG_GENERIC );
	}

	// If the hook is not attached to the player, constantly copy
	// copy the target's velocity. Velocity copying DOES NOT work properly
	// for a hooked client. 
	else
		pev->velocity = pEnemy->pev->velocity;

	pev->nextthink = gpGlobals->time + 0.1;
}

void CBasePlayer::Service_Grapple()
{
        Vector hook_dir;
	CBaseEntity *pEnemy = CBaseEntity::Instance( pev->enemy );

	// drop the hook if player lets go of button
	if( !( pev->button & IN_ATTACK ) )
	{
		if( m_iQuakeWeapon == IT_EXTRA_WEAPON ) 
		{
			m_ppHook->Reset_Grapple();
				return;
		}
	}

	if( m_ppHook->pev->enemy != NULL )
	{
		// If hooked to a player, track them directly!
		if( pEnemy->IsPlayer() )
		{
			pEnemy = CBaseEntity::Instance( pev->enemy );
			hook_dir = ( pEnemy->pev->origin - pev->origin );
		}
		// else, track to hook
		else
			hook_dir = ( m_ppHook->pev->origin - pev->origin );
	
		pev->speed = 750;
		pev->velocity = ( hook_dir.Normalize() * pev->speed );

		if( m_ppHook->m_flNextIdleTime <= gpGlobals->time && hook_dir.Length() <= 50 )
		{
			//No sparks underwater
			if( m_ppHook->pev->waterlevel == 0 )
				UTIL_Sparks( m_ppHook->pev->origin );

			STOP_SOUND( edict(), CHAN_WEAPON, "weapons/grpull.wav" );
			EMIT_SOUND( ENT( m_ppHook->pev ), CHAN_WEAPON, "weapons/grhang.wav", 1, ATTN_NORM );

			m_ppHook->m_flNextIdleTime = gpGlobals->time + RANDOM_LONG( 1, 3 );

			PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, 
			edict(), g_usCable, 0, (float *)&g_vecZero, (float *)&g_vecZero, 
			0.0, 0.0, m_ppHook->entindex(), pev->team, 1, 0 );
		}
		else if( m_ppHook->m_flNextIdleTime <= gpGlobals->time )
		{
			// No sparks underwater
			if( m_ppHook->pev->waterlevel == 0 )
				UTIL_Sparks( m_ppHook->pev->origin );

			STOP_SOUND( edict(), CHAN_WEAPON, "weapons/grfire.wav" );
			EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/grpull.wav", 1, ATTN_NORM );
			m_ppHook->m_flNextIdleTime = gpGlobals->time + RANDOM_LONG( 1, 3 );
		}
	}
}

void CGrapple::OnAirThink()
{
	TraceResult tr;

	CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );

	if( !( pOwner->pev->button & IN_ATTACK ) )
	{
		Reset_Grapple();
		return;
	}

	UTIL_TraceLine( pev->origin, pOwner->pev->origin, ignore_monsters, ENT( pev ), &tr );

	if( tr.flFraction < 1.0 )
	{
		Reset_Grapple();
		return;
	}

	pev->nextthink = gpGlobals->time + 0.5;
}

void CGrapple::Spawn()
{
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT( pev ),"models/hook.mdl" );

	SetTouch( &CGrapple::GrappleTouch );
	SetThink( &CGrapple::OnAirThink );

	pev->nextthink = gpGlobals->time + 0.1;
}

LINK_ENTITY_TO_CLASS( hook, CGrapple ); 

void CBasePlayer::Throw_Grapple()
{
	if( m_bHook_Out )
		return;

	CGrapple *pHookCBEnt = NULL;
	pHookCBEnt = (CGrapple*)CBaseEntity::Create( "hook", pev->origin, pev->angles, NULL );

	if( pHookCBEnt )
	{
		m_ppHook = pHookCBEnt;
		
		m_ppHook->pev->owner = edict();
		
		UTIL_MakeVectors( pev->v_angle );

		UTIL_SetOrigin( m_ppHook->pev, pev->origin + gpGlobals->v_forward * 16 + Vector( 0, 0, 16 ) );
		UTIL_SetSize( m_ppHook->pev, g_vecZero, g_vecZero );

		EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/grfire.wav", 1, ATTN_NORM );

		// Make if fly forward
		m_ppHook->pev->velocity = gpGlobals->v_forward * 1000;
		// And make the hook face forward too!
		m_ppHook->pev->angles = UTIL_VecToAngles( gpGlobals->v_forward );
		m_ppHook->pev->fixangle = TRUE;

		PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, 
		edict(), g_usCable, 0, (float*)&g_vecZero, (float*)&g_vecZero, 
		0.0, 0.0, m_ppHook->entindex(), pev->team, 0, 0 );
		
		m_bHook_Out = TRUE;
	}
}
