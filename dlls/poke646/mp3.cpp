/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "mp3.h"

extern int gmsgPlayMP3;
#define SF_AMBIENTMP3_LOOP	1

LINK_ENTITY_TO_CLASS( ambient_mp3, CAmbientMP3 )

CAmbientMP3 *CAmbientMP3::AmbientMP3Create( const char *pszSound )
{
	// Create a new entity with CCrossbowBolt private data
	CAmbientMP3 *pMPlayer = GetClassPtr( (CAmbientMP3 *)NULL );
	pMPlayer->pev->classname = MAKE_STRING( "ambient_mp3" );
	pMPlayer->pev->message = MAKE_STRING( pszSound );
	pMPlayer->Spawn();

	return pMPlayer;
}

void CAmbientMP3::Spawn( void )
{
	if( pev->message && strlen( STRING( pev->message ) ) >= 1 )
	{
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_NONE;
		SetUse( &CAmbientMP3::ToggleUse );
	}
	else
	{
		ALERT( at_console, "ambient_mp3 without soundfile at: %f, %f, %f, removing self...\n", (double)pev->origin.x, (double)pev->origin.y, (double)pev->origin.z );
		UTIL_Remove( this );
	}
}

void CAmbientMP3::ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	edict_t *pClient;
	const char *pszSound;
	int i;
	BOOL loop;

	loop = pev->spawnflags & SF_AMBIENTMP3_LOOP;
	pszSound = STRING( pev->message );

	if( FStrEq( pszSound, "#po_soundtrack#" ) )
	{
		pszSound = 0;

		for( i = 0; i < g_soundtrackCount; i++ )
		{
			if( FStrEq( g_soundtracklist[i].mapname, STRING( gpGlobals->mapname ) ) )
			{
				pszSound = g_soundtracklist[i].soundfile;
				loop = g_soundtracklist[i].loop;
				break;
			}
		}
	}

	if( !pszSound )
	{
		UTIL_Remove( this );
		return;
	}

	// manually find the single player.
	pClient = g_engfuncs.pfnPEntityOfEntIndex( 1 );

	if( pClient )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgPlayMP3, NULL, pClient );
			WRITE_STRING( pszSound );
			WRITE_BYTE( loop );
		MESSAGE_END();
	}

	UTIL_Remove( this );
}

