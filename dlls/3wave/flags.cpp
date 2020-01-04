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
/*****************************************************
******************************************************
                THREEWAVE CTF FLAG CODE
******************************************************
*****************************************************/

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"items.h"
#include	"threewave_gamerules.h"
#include	"flags.h"

unsigned short g_usCarried;
unsigned short g_usFlagSpawn;
enum Flag_Anims
{
	ON_GROUND = 0,
	NOT_CARRIED,
	CARRIED,
	WAVE_IDLE,
	FLAG_POSITION
}; 

void CItemFlag::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/flag.mdl" );

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );

	SetThink( &CItemFlag::FlagThink );
	SetTouch( &CItemFlag::FlagTouch ); 

	pev->nextthink = gpGlobals->time + 0.3f;

	//Set the Skin based on the team.
	pev->skin = pev->team;

	Dropped = FALSE;
	m_flDroppedTime = 0.0;

	pev->sequence = NOT_CARRIED;
	pev->framerate = 1.0; 

	// if( !DROP_TO_FLOOR( ENT( pev ) ) )
	//	ResetFlag( pev->team );
} 

void CItemFlag::FlagTouch( CBaseEntity *pToucher )
{
	if( !pToucher )
		return;

	if( !pToucher->IsPlayer() )
		return;

	if( FBitSet( pev->effects, EF_NODRAW ) )
		return;
	
	if( pToucher->pev->health <= 0 )
		 return;

	if( pToucher->pev->team == 0 )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pToucher;

	//Same team as the flag
	if( pev->team == pToucher->pev->team )
	{
		//Flag is dropped, let's return it
		if( Dropped )
		{
			Dropped = FALSE;

			pPlayer->AddPoints( TEAM_CAPTURE_RECOVERY_BONUS, TRUE );

			if( pPlayer->pev->team == RED )
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Returned_Red_Flag\"\n",  
					STRING( pPlayer->pev->netname ), 
					GETPLAYERUSERID( pPlayer->edict() ),
					GETPLAYERAUTHID( pPlayer->edict() ),
					GetTeamName( pPlayer->pev->team ) );

				if( ( (CThreeWave*)g_pGameRules )->iBlueFlagStatus == BLUE_FLAG_STOLEN )
				{
					for( int i = 1; i <= gpGlobals->maxClients; i++ )
					{
						CBasePlayer *pTeamMate = (CBasePlayer*)UTIL_PlayerByIndex( i );

						if( pTeamMate )
						{
							if( pTeamMate->m_bHasFlag )
							{
								pTeamMate->pFlagReturner = pPlayer;
								pTeamMate->m_flFlagReturnTime = gpGlobals->time + TEAM_CAPTURE_RETURN_FLAG_ASSIST_TIMEOUT;
							}
						}
					}
				}
			}

			if( pPlayer->pev->team == BLUE )
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Returned_Blue_Flag\"\n",  
					STRING( pPlayer->pev->netname ), 
					GETPLAYERUSERID( pPlayer->edict() ),
					GETPLAYERAUTHID( pPlayer->edict() ),
					GetTeamName( pPlayer->pev->team ) );

				if( ( (CThreeWave*)g_pGameRules )->iRedFlagStatus == RED_FLAG_STOLEN )
				{
					for( int i = 1; i <= gpGlobals->maxClients; i++ )
					{
						CBasePlayer *pTeamMate = (CBasePlayer*)UTIL_PlayerByIndex( i );

						if( pTeamMate )
						{
							if( pTeamMate->m_bHasFlag )
							{
								pTeamMate->pFlagReturner = pPlayer;
								pTeamMate->m_flFlagReturnTime = gpGlobals->time + TEAM_CAPTURE_RETURN_FLAG_ASSIST_TIMEOUT;
							}
						}
					}
				}
			}

			//Back at home!
			ResetFlag( pev->team );
            
			MESSAGE_BEGIN( MSG_ALL, gmsgCTFMsgs, NULL );
				if( pev->team == RED )
					WRITE_BYTE( RED_FLAG_RETURNED_PLAYER );
				else if( pev->team == BLUE )
					WRITE_BYTE( BLUE_FLAG_RETURNED_PLAYER );
				WRITE_STRING( STRING( pToucher->pev->netname ) );
			MESSAGE_END();

			// Remove this one
			UTIL_Remove( this );
			return;
		}
		// Not Dropped, means it's the one in our base
		else
		{
			// We have the enemy flag!
			// Capture it!
			if( pPlayer->m_bHasFlag )
			{
				if( pev->team == RED )
					Capture( pPlayer, BLUE );
				else if( pev->team == BLUE )
					Capture( pPlayer, RED );

				PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, 
				pPlayer->edict(), g_usCarried, 0, g_vecZero, g_vecZero, 
				0.0, 0.0, pPlayer->entindex(), pPlayer->pev->team, 1, 0 );
				return;
			}
		}
	}
	else 
	{
		if( Dropped )
		{
			MESSAGE_BEGIN( MSG_ALL, gmsgCTFMsgs, NULL );
				if( pev->team == RED )
					WRITE_BYTE( RED_FLAG_STOLEN );
				else if( pev->team == BLUE )
					WRITE_BYTE( BLUE_FLAG_STOLEN );
				WRITE_STRING( STRING( pToucher->pev->netname ) );
			MESSAGE_END();

			pPlayer->m_bHasFlag = TRUE;

			CBaseEntity *pEnt = NULL;

			if( pev->team == RED )
			{
				pEnt = CBaseEntity::Create( "carried_flag_team1", pPlayer->pev->origin, pPlayer->pev->angles, pPlayer->edict() );

				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Picked_Up_Red_Flag\"\n",  
					STRING( pPlayer->pev->netname ), 
					GETPLAYERUSERID( pPlayer->edict() ),
					GETPLAYERAUTHID( pPlayer->edict() ),
					GetTeamName( pPlayer->pev->team ) );
			}
			else if( pev->team == BLUE )
			{
				pEnt = CBaseEntity::Create( "carried_flag_team2", pPlayer->pev->origin, pPlayer->pev->angles, pPlayer->edict() );

				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Picked_Up_Blue_Flag\"\n",  
					STRING( pPlayer->pev->netname ), 
					GETPLAYERUSERID( pPlayer->edict() ),
					GETPLAYERAUTHID( pPlayer->edict() ),
					GetTeamName( pPlayer->pev->team ) );
			}

			CCarriedFlag *pCarriedFlag = (CCarriedFlag *)pEnt;
			pCarriedFlag->Owner = pPlayer;

			PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, 
			pPlayer->edict(), g_usCarried, 0, g_vecZero, g_vecZero, 
			0.0, 0.0, pPlayer->entindex(), pPlayer->pev->team, 0, 0 );

			UTIL_Remove( this );
		}
		else
		{
			pev->effects |= EF_NODRAW;
			MESSAGE_BEGIN( MSG_ALL, gmsgCTFMsgs, NULL );
				if( pev->team == RED )
					WRITE_BYTE( RED_FLAG_STOLEN );
				else if( pev->team == BLUE )
					WRITE_BYTE( BLUE_FLAG_STOLEN );
				WRITE_STRING( STRING( pToucher->pev->netname ) );
			MESSAGE_END();

			pPlayer->m_bHasFlag = TRUE;
			pPlayer->m_flCarrierPickupTime = gpGlobals->time + TEAM_CAPTURE_CARRIER_FLAG_SINCE_TIMEOUT;

			CBaseEntity *pEnt = NULL;

			if( pev->team == RED )
			{
				pEnt = CBaseEntity::Create( "carried_flag_team1", pev->origin, pev->angles, pPlayer->edict() );

				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Stole_Red_Flag\"\n",  
					STRING( pPlayer->pev->netname ), 
					GETPLAYERUSERID( pPlayer->edict() ),
					GETPLAYERAUTHID( pPlayer->edict() ),
					GetTeamName( pPlayer->pev->team ) );
			}
			else if( pev->team == BLUE )
			{
				pEnt = CBaseEntity::Create( "carried_flag_team2", pev->origin, pev->angles, pPlayer->edict() );

				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Stole_Blue_Flag\"\n",  
					STRING( pPlayer->pev->netname ), 
					GETPLAYERUSERID( pPlayer->edict() ),
					GETPLAYERAUTHID( pPlayer->edict() ),
					GetTeamName( pPlayer->pev->team ) );
			}

			CCarriedFlag *pCarriedFlag = (CCarriedFlag *)pEnt;
			pCarriedFlag->Owner = pPlayer;

			PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, 
			pPlayer->edict(), g_usCarried, 0, g_vecZero, g_vecZero, 
			0.0, 0.0, pPlayer->entindex(), pPlayer->pev->team, 0, 0 );
		}
		
		( (CThreeWave*)g_pGameRules )->m_flFlagStatusTime = gpGlobals->time + 0.1f;
	}
}

void CItemFlag::Capture( CBasePlayer *pPlayer, int iTeam )
{
	CBaseEntity *pFlag1 = NULL; 

	MESSAGE_BEGIN( MSG_ALL, gmsgCTFMsgs, NULL );
		if( iTeam == RED )
			WRITE_BYTE( RED_FLAG_CAPTURED );
		else if ( iTeam == BLUE )
			WRITE_BYTE( BLUE_FLAG_CAPTURED );
		WRITE_STRING( STRING( pPlayer->pev->netname ) );
	MESSAGE_END();

	if( pPlayer->pFlagCarrierKiller )
	{
		if( pPlayer->m_flFlagCarrierKillTime > gpGlobals->time )
		{
			UTIL_ClientPrintAll( HUD_PRINTNOTIFY, STRING( pPlayer->pFlagCarrierKiller->pev->netname ) );
			UTIL_ClientPrintAll( HUD_PRINTNOTIFY, " gets an assist for fragging the flag carrier!\n" );

			pPlayer->pFlagCarrierKiller->AddPoints( TEAM_CAPTURE_FRAG_CARRIER_ASSIST_BONUS, TRUE );
			pPlayer->pFlagCarrierKiller = NULL;
			pPlayer->m_flFlagCarrierKillTime = 0.0;
		}
	}

	if( pPlayer->pFlagReturner )
	{
		if( pPlayer->m_flFlagReturnTime > gpGlobals->time )
		{
			UTIL_ClientPrintAll( HUD_PRINTNOTIFY, STRING( pPlayer->pFlagReturner->pev->netname ) );
			UTIL_ClientPrintAll( HUD_PRINTNOTIFY, " gets an assist for returning his flag!\n");

			pPlayer->pFlagReturner->AddPoints( TEAM_CAPTURE_RETURN_FLAG_ASSIST_BONUS, TRUE );
			pPlayer->pFlagReturner = NULL;
			pPlayer->m_flFlagReturnTime = 0.0;
		}
	}

	if( iTeam != pPlayer->pev->team )
	{
		if( iTeam == RED )
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Captured_Red_Flag\"\n",  
					STRING( pPlayer->pev->netname ), 
					GETPLAYERUSERID( pPlayer->edict() ),
					GETPLAYERAUTHID( pPlayer->edict() ),
					GetTeamName( pPlayer->pev->team ) );
		}
		else
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Captured_Blue_Flag\"\n",  
					STRING( pPlayer->pev->netname ), 
					GETPLAYERUSERID( pPlayer->edict() ),
					GETPLAYERAUTHID( pPlayer->edict() ),
					GetTeamName( pPlayer->pev->team ) );
		}
	}

	if( iTeam == RED )
	{
		( (CThreeWave *)g_pGameRules )->iBlueTeamScore++;

		while( ( pFlag1 = UTIL_FindEntityByClassname( pFlag1, "carried_flag_team1" ) ) != NULL )
		{
			if( pFlag1 )
				UTIL_Remove( pFlag1 );
		}
	}
	else if( iTeam == BLUE )
	{
		( (CThreeWave*)g_pGameRules )->iRedTeamScore++;

		while( ( pFlag1 = UTIL_FindEntityByClassname( pFlag1, "carried_flag_team2" ) ) != NULL )
		{
			if ( pFlag1 )
				UTIL_Remove( pFlag1 );
		}
	}

	pPlayer->m_bHasFlag = FALSE;
	pPlayer->AddPoints( TEAM_CAPTURE_CAPTURE_BONUS, TRUE );

	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pTeamMate = UTIL_PlayerByIndex( i );

		if( pTeamMate )
		{
			if( pTeamMate->pev->team == pPlayer->pev->team )
				pTeamMate->AddPoints( TEAM_CAPTURE_TEAM_BONUS, TRUE );
		}
	}

	ResetFlag( iTeam );
} 

void CItemFlag::Materialize()
{
	if ( pev->effects & EF_NODRAW )
	{
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, 
	edict(), g_usFlagSpawn, 0, g_vecZero, g_vecZero, 
	0.0, 0.0, pev->team, 0, 0, 0 );

	Dropped = FALSE;

	SetTouch( &CItemFlag::FlagTouch );
	SetThink( &CItemFlag::FlagThink );
}

void CItemFlag::ResetFlag( int iTeam )
{
	CBaseEntity *pFlag1 = NULL;

	if( iTeam == BLUE )
	{
		while( ( pFlag1 = UTIL_FindEntityByClassname( pFlag1, "item_flag_team2" ) ) != NULL )
		{
			CItemFlag *pFlag2 = (CItemFlag*)pFlag1;

			if( pFlag2->Dropped )
				continue;

			if( pFlag2->pev->effects & EF_NODRAW )
				 pFlag2->Materialize(); 
		}
	}
	else if( iTeam == RED )
	{
		while( ( pFlag1 = UTIL_FindEntityByClassname( pFlag1, "item_flag_team1" ) ) != NULL )
		{
			CItemFlag *pFlag2 = (CItemFlag*)pFlag1;

			if( pFlag2->Dropped )
				continue;

			if( pFlag2->pev->effects & EF_NODRAW )
				pFlag2->Materialize(); 
		}
	}

	( (CThreeWave*)g_pGameRules )->m_flFlagStatusTime = gpGlobals->time + 0.1f;
}

void CItemFlag::FlagThink()
{
   	if( Dropped )
	{
		if( m_flDroppedTime <= gpGlobals->time )
		{
			ResetFlag( pev->team );
			MESSAGE_BEGIN( MSG_ALL, gmsgCTFMsgs, NULL );
				if( pev->team == RED )
					WRITE_BYTE( RED_FLAG_RETURNED );
				else if( pev->team == BLUE )
					WRITE_BYTE( BLUE_FLAG_RETURNED );
				WRITE_STRING( "" );
			MESSAGE_END();

			UTIL_Remove( this );
			return;
		}
	}

	//Using 0.2 just in case we might lag the server.
	pev->nextthink = gpGlobals->time + 0.2f;
} 

void CItemFlag::Precache()
{
	PRECACHE_MODEL( "models/flag.mdl" );
	PRECACHE_SOUND( "ctf/flagcap.wav" );
	PRECACHE_SOUND( "ctf/flagtk.wav" );
	PRECACHE_SOUND( "ctf/flagret.wav" );
} 

class CItemFlagTeam1 : public CItemFlag
{
	void Spawn()
	{
		pev->classname = MAKE_STRING( "item_flag_team1" );
		pev->team = RED;
		CItemFlag::Spawn();
	}
}; 

class CItemFlagTeam2 : public CItemFlag
{
	void Spawn()
	{
		pev->classname = MAKE_STRING( "item_flag_team2" );
		pev->team = BLUE;
		CItemFlag::Spawn();
	}
}; 

LINK_ENTITY_TO_CLASS( item_flag_team1, CItemFlagTeam1 )
LINK_ENTITY_TO_CLASS( ctf_redflag, CItemFlagTeam1 ) 
LINK_ENTITY_TO_CLASS( item_flag_team2, CItemFlagTeam2 )
LINK_ENTITY_TO_CLASS( ctf_blueflag, CItemFlagTeam2 ) 

void CCarriedFlag::Spawn()
{
	Precache();

	SET_MODEL( ENT( pev ), "models/flag.mdl" );
	UTIL_SetOrigin( pev, pev->origin ); 

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT; 
	pev->effects |= EF_NODRAW; 
	pev->sequence = WAVE_IDLE;
	pev->framerate = 1.0;

	/*
	if( pev->team == RED )
		pev->skin = 1;
	else if( pev->team == BLUE )
		pev->skin = 2;*/
	pev->skin = pev->team;

	m_iOwnerOldVel = 0;

	SetThink( &CCarriedFlag::FlagThink );
	pev->nextthink = gpGlobals->time + 0.1f;
} 

void CCarriedFlag::Precache()
{
	PRECACHE_MODEL( "models/flag.mdl" );
}

void CCarriedFlag::FlagThink()
{
	// Make it visible
	pev->effects &= ~EF_NODRAW;

	// And let if follow
	pev->aiment = ENT( Owner->pev );
	pev->movetype = MOVETYPE_FOLLOW;

	// If owner is death or lost flag, remove
	if( !Owner->IsAlive() || !Owner->m_bHasFlag )
		UTIL_Remove( this );
	else
	{
		// If owners speed is low, go in idle mode
		if( Owner->pev->velocity.Length() <= 75 && pev->sequence != WAVE_IDLE )
		{
			pev->sequence = WAVE_IDLE;
		}
		// Else let the flag go wild
		else if( Owner->pev->velocity.Length() >= 75 && pev->sequence != CARRIED )
		{
			pev->sequence = CARRIED;
		}

		pev->frame += pev->framerate;

		if( pev->frame < 0.0f || pev->frame >= 256.0f )
		{
			pev->frame -= (int)( pev->frame / 256.0f ) * 256.0f;
		}
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

class CCarriedFlagTeam1 : public CCarriedFlag
{
	void Spawn()
	{
		pev->team = RED;
		CCarriedFlag::Spawn();
	}
}; 

class CCarriedFlagTeam2 : public CCarriedFlag
{
	void Spawn()
	{
		pev->team = BLUE;
		CCarriedFlag::Spawn();
	}
};

LINK_ENTITY_TO_CLASS( carried_flag_team1, CCarriedFlagTeam1 )
LINK_ENTITY_TO_CLASS( carried_flag_team2, CCarriedFlagTeam2 )
