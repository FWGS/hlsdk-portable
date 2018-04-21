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
***************************************
****************************************
				RUNES
****************************************
***************************************/

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"threewave_gamerules.h"
#include	"runes.h"

extern bool g_bSpawnedRunes;

/*----------------------------------------------------------------------
  The Rune Game modes

  Rune 1 - Earth Magic
	  resistance
  Rune 2 - Black Magic
	  strength
  Rune 3 - Hell Magic
	  haste
  Rune 4 - Elder Magic
	  regeneration

 ----------------------------------------------------------------------*/

BOOL IsRuneSpawnPointValid( CBaseEntity *pSpot )
{
	CBaseEntity *ent = NULL;

	while( ( ent = UTIL_FindEntityInSphere( ent, pSpot->pev->origin, 128 ) ) != NULL )
	{
		//Try not to spawn it near other runes.
		if( ent->Classify() == CLASS_RUNE )
			return FALSE;
	}

	return TRUE;
}

edict_t *RuneSelectSpawnPoint()
{
	CBaseEntity *pSpot = NULL;

	// Randomize the start spot
	for( int i = RANDOM_LONG( 1, 5 ); i > 0; i-- )
		pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
	if( !pSpot ) // skip over the null point
		pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );

	CBaseEntity *pFirstSpot = pSpot;

	do
	{
		if( pSpot )
		{
			if( IsRuneSpawnPointValid( pSpot ) )
			{
				if( pSpot->pev->origin == g_vecZero )
				{
					pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
					continue;
				}
				// if so, go to pSpot
				goto ReturnSpot;
			}
		}
		// increment pSpot
		pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" );
	} while ( pSpot != pFirstSpot ); // loop if we're not back to the start

	// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
	if( pSpot )
		goto ReturnSpot;

	// If startspot is set, (re)spawn there.
	if( FStringNull( gpGlobals->startspot ) || !strlen( STRING( gpGlobals->startspot ) ) )
	{
		pSpot = UTIL_FindEntityByClassname( NULL, "info_player_start" );
	}
	else
	{
		pSpot = UTIL_FindEntityByTargetname( NULL, STRING( gpGlobals->startspot ) );
	}	

ReturnSpot:
	if( !pSpot )
	{
		ALERT( at_error, "PutClientInServer: no info_player_start on level" );
		return INDEXENT( 0 );
	}
	return pSpot->edict();
}

void VectorScale( const float *in, float scale, float *out )
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}

void G_ProjectSource( vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result )
{
	result[0] = point[0] + forward[0] * distance[0] + right[0] * distance[1];
	result[1] = point[1] + forward[1] * distance[0] + right[1] * distance[1];
	result[2] = point[2] + forward[2] * distance[0] + right[2] * distance[1] + distance[2];
}

#define VectorSet(v, x, y, z)	(v[0]=(x), v[1]=(y), v[2]=(z))

void DropRune( CBasePlayer *pPlayer )
{
	TraceResult tr;

	// do they even have a rune?
	if( pPlayer->m_iRuneStatus == 0 )
		return;

	// Make Sure there's enough room to drop the rune here
	// This is so hacky ( the reason why we are doing this), and I hate it to death.
	UTIL_MakeVectors( pPlayer->pev->v_angle );
	Vector vecSrc	= pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 32;
	UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, human_hull, ENT( pPlayer->pev ), &tr );

	if( tr.flFraction != 1 )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Not enough room to drop the rune here." );
		return;
	}

	if( pPlayer->m_iRuneStatus )
	{
		CItemRune *pRune = (CItemRune*)CBaseEntity::Create( g_RuneEntityName[pPlayer->m_iRuneStatus], pPlayer->pev->origin, pPlayer->pev->angles, pPlayer->edict() );

		if( pRune )
			pRune->dropped = true;

		if( pPlayer->m_iRuneStatus == ITEM_RUNE3_FLAG )
			g_engfuncs.pfnSetClientMaxspeed( ENT( pPlayer->pev ), PLAYER_MAX_SPEED ); // Reset Haste player speed to normal

		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Dropped_%s\"\n", 
			STRING(pPlayer->pev->netname),
			GETPLAYERUSERID( pPlayer->edict() ),
			GETPLAYERAUTHID( pPlayer->edict() ),
			pPlayer->m_szTeamName,
			g_RuneName[pPlayer->m_iRuneStatus] );
		pPlayer->m_iRuneStatus = 0;
	}

	MESSAGE_BEGIN( MSG_ONE, gmsgRuneStatus, NULL, pPlayer->pev );
		WRITE_BYTE( pPlayer->m_iRuneStatus );
	MESSAGE_END();
}

void CItemRune::Spawn()
{
	m_bTouchable = FALSE;

	dropped = false;

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	vec3_t forward, right, up;
	Vector vecAngles;

	UTIL_SetSize( pev, Vector( -15, -15, -15 ), Vector( 15, 15, 15 ) ); 

	pev->angles.z = pev->angles.x = 0;
	pev->angles.y = RANDOM_LONG ( 0, 360 );

	//If we got an owner, it means we are either dropping the flag or diying and letting it go.
	if( pev->owner )
		vecAngles = pev->owner->v.angles;
	else
		vecAngles = pev->angles;

	g_engfuncs.pfnAngleVectors( vecAngles, forward, right, up );

	UTIL_SetOrigin( pev, pev->origin );
	
	pev->velocity = ( forward * 400 ) + ( up * 200 );
	
	if( pev->owner == NULL )
	{
		pev->origin.z += 16;
		pev->velocity.z = 300;
	}
	
	pev->owner = NULL;

	SetTouch( &CItemRune::RuneTouch );

	pev->nextthink = gpGlobals->time + 1; // if no one touches it in two minutes,
											// respawn it somewhere else, so inaccessible 
											// ones will come 'back'
	SetThink( &CItemRune::MakeTouchable );
}

void CItemRune::MakeTouchable()
{
	m_bTouchable = TRUE;
	pev->nextthink = gpGlobals->time + 120; // if no one touches it in two minutes,
											// respawn it somewhere else, so inaccessible 
											// ones will come 'back'
	SetThink( &CItemRune::RuneRespawn );
}

void CItemRune::RuneTouch( CBaseEntity *pOther )
{
	// No toucher?
	if( !pOther )
		return;

	// Not a player?
	if( !pOther->IsPlayer() )
		return;

	// DEAD?!
	if( pOther->pev->health <= 0 )
		 return;

	// Spectating?
	if( pOther->pev->movetype == MOVETYPE_NOCLIP )
		 return;

	CBasePlayer *pPlayer = (CBasePlayer*)pOther;

	// Only one per customer
	if( pPlayer->m_iRuneStatus )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "You already have a rune!\n" );
		return;
	}

	if( !m_bTouchable )
		return;

	pPlayer->m_iRuneStatus = m_iRuneFlag; //Add me the rune flag

	PrintTouchMessage( pPlayer );

	EMIT_SOUND( ENT( pev ), CHAN_ITEM, "weapons/lock4.wav", 1, ATTN_NORM );

	//Update my client side rune hud thingy.
	MESSAGE_BEGIN( MSG_ONE, gmsgRuneStatus, NULL, pOther->pev );
		WRITE_BYTE( pPlayer->m_iRuneStatus );
	MESSAGE_END();

	// And Remove this entity
	UTIL_Remove( this );
}

void CItemRune::RuneRespawn()
{
	edict_t *pentSpawnSpot;
	vec3_t vOrigin;

	pentSpawnSpot = RuneSelectSpawnPoint();
	vOrigin = VARS( pentSpawnSpot )->origin;

	UTIL_SetOrigin( pev, vOrigin );

	if( dropped )
		PrintRespawnMessage();
	    
	Spawn();
}

void CResistRune::PrintTouchMessage( CBasePlayer *pPlayer )
{
	ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "You got the rune of Resistance!\n" );

	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Found_ResistRune\"\n",
		STRING(pPlayer->pev->netname),
		GETPLAYERUSERID( pPlayer->edict() ),
		GETPLAYERAUTHID( pPlayer->edict() ),
		pPlayer->m_szTeamName );
}

void CResistRune::PrintRespawnMessage()
{
	UTIL_LogPrintf( "\"<-1><><>\" triggered \"Respawn_ResistRune\"\n" );
}

void CResistRune::Spawn()
{
	SET_MODEL( ENT( pev ), "models/rune_resist.mdl" );
	m_iRuneFlag = ITEM_RUNE1_FLAG;
	CItemRune::Spawn();
}

LINK_ENTITY_TO_CLASS( item_rune1, CResistRune ); 

void CStrengthRune::PrintTouchMessage( CBasePlayer *pPlayer )
{
	ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "You got the rune of Strength!\n" );
 
	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Found_StrengthRune\"\n",
		STRING( pPlayer->pev->netname ),
		GETPLAYERUSERID( pPlayer->edict() ),
		GETPLAYERAUTHID( pPlayer->edict() ),
		pPlayer->m_szTeamName );
}

void CStrengthRune::PrintRespawnMessage()
{
	UTIL_LogPrintf( "\"<-1><><>\" triggered \"Respawn_StrengthRune\"\n" );
}

void CStrengthRune::Spawn()
{
	SET_MODEL( ENT( pev ), "models/rune_strength.mdl" );
	m_iRuneFlag = ITEM_RUNE2_FLAG;
	CItemRune::Spawn();
}

LINK_ENTITY_TO_CLASS( item_rune2, CStrengthRune )

void CHasteRune::PrintTouchMessage( CBasePlayer *pPlayer )
{
	ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "You got the rune of Haste!\n" );

	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Found_HasteRune\"\n",
		STRING( pPlayer->pev->netname ),
		GETPLAYERUSERID( pPlayer->edict() ),
		GETPLAYERAUTHID( pPlayer->edict() ),
		pPlayer->m_szTeamName );

	g_engfuncs.pfnSetClientMaxspeed( ENT( pPlayer->pev ), ( PLAYER_MAX_SPEED * 1.25 ) ); // 25% more speed
}

void CHasteRune::PrintRespawnMessage()
{
	UTIL_LogPrintf( "\"<-1><><>\" triggered triggered \"Respawn_HasteRune\"\n" );
}

void CHasteRune::Spawn()
{
	SET_MODEL( ENT( pev ), "models/rune_haste.mdl" );
	m_iRuneFlag = ITEM_RUNE3_FLAG;
	CItemRune::Spawn();
}

LINK_ENTITY_TO_CLASS( item_rune3, CHasteRune ) 

void CRegenRune::PrintTouchMessage( CBasePlayer *pPlayer )
{
	ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "You got the rune of Regeneration!\n" );

	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Found_RegenRune\"\n",
		STRING( pPlayer->pev->netname ),
		GETPLAYERUSERID( pPlayer->edict() ),
		GETPLAYERAUTHID( pPlayer->edict() ),
		pPlayer->m_szTeamName );
}

void CRegenRune::PrintRespawnMessage()
{
	UTIL_LogPrintf( "\"<-1><><>\" triggered triggered \"Respawn_RegenRune\"\n" );
}

void CRegenRune::Spawn()
{
	SET_MODEL( ENT( pev ), "models/rune_regen.mdl" );
	m_iRuneFlag = ITEM_RUNE4_FLAG;
	CItemRune::Spawn();
}

LINK_ENTITY_TO_CLASS( item_rune4, CRegenRune )

/*
================
SpawnRunes
spawn all the runes
self is the entity that was created for us, we remove it
================
*/
void SpawnRunes()
{
	if( g_bSpawnedRunes )
		return;

	edict_t *pentSpawnSpot;

	pentSpawnSpot = RuneSelectSpawnPoint();
	CBaseEntity::Create( "item_rune1", VARS(pentSpawnSpot)->origin, VARS( pentSpawnSpot )->angles, NULL );
	
	pentSpawnSpot = RuneSelectSpawnPoint();
	CBaseEntity::Create( "item_rune2", VARS(pentSpawnSpot)->origin, VARS( pentSpawnSpot )->angles, NULL );

	pentSpawnSpot = RuneSelectSpawnPoint();
	CBaseEntity::Create( "item_rune3", VARS(pentSpawnSpot)->origin, VARS( pentSpawnSpot )->angles, NULL );
	
	pentSpawnSpot = RuneSelectSpawnPoint();
	CBaseEntity::Create( "item_rune4", VARS(pentSpawnSpot)->origin, VARS( pentSpawnSpot )->angles, NULL );

	g_bSpawnedRunes = TRUE;
}
