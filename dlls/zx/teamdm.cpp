#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"game.h"
//#include	"voice_gamemgr.h"
#include	"teamdm.h"

extern int gmsgScoreInfo;
extern int gmsgTeamInfo;

CRulesTeamDM :: CRulesTeamDM( )
{
	m_DisableDeathMessages = FALSE;
	m_DisableDeathPenalty = FALSE;
}

void CRulesTeamDM :: ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, BOOL bKill, BOOL bGib )
{
	int damageFlags = DMG_GENERIC;
	int clientIndex = pPlayer->entindex();

	if ( !bGib )
	{
		damageFlags |= DMG_NEVERGIB;
	}
	else
	{
		damageFlags |= DMG_ALWAYSGIB;
	}

	if ( bKill )
	{
		m_DisableDeathMessages = TRUE;
		m_DisableDeathPenalty = TRUE;

		entvars_t *pevWorld = VARS( INDEXENT(0) );
		pPlayer->TakeDamage( pevWorld, pevWorld, 900, damageFlags );

		m_DisableDeathMessages = FALSE;
		m_DisableDeathPenalty = FALSE;
	}

	strncpy( pPlayer->m_szTeamName, pTeamName, TEAM_NAME_LENGTH );
	g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "team", pPlayer->m_szTeamName );

	// notify everyone's HUD of the team change

	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( clientIndex );
		WRITE_STRING( pPlayer->m_szTeamName );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( clientIndex );
		WRITE_SHORT( pPlayer->pev->frags );
		WRITE_SHORT( pPlayer->m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( g_pGameRules->GetTeamIndex( pPlayer->m_szTeamName ) + 1 );
	MESSAGE_END();
}

//extern int gmsgVGUIMenu;
extern void respawn( entvars_t* pev, BOOL fCopyCorpse );
//extern CVoiceGameMgr g_VoiceGameMgr;

BOOL CRulesTeamDM :: ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
	// UPDATE: this gets rid of the "vban" and "vModEnable" warnings in the console

	/*if ( g_VoiceGameMgr.ClientCommand( pPlayer, pcmd ) )
		return TRUE;

	if ( FStrEq( pcmd, "changeteam" ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgVGUIMenu, NULL, pPlayer->edict() );
			WRITE_BYTE( 2 );
		MESSAGE_END();

		return TRUE;
	}	
*/
	if ( FStrEq( pcmd, "jointeam" ) )
	{
		if ( CMD_ARGC( ) > 1 )
		{
			pPlayer->m_iNextTeam = atoi( CMD_ARGV( 1 ) );

			// team change is handled in PlayerSpawn, so respawn!

			respawn( pPlayer->pev, FALSE );
		}

		return TRUE;
	}

	return FALSE;
}

extern int gmsgDeathMsg;

void CRulesTeamDM :: DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor )
{
	if ( m_DisableDeathMessages )
		return;
	
	if ( pVictim && pKiller && pKiller->flags & FL_CLIENT )
	{
		CBasePlayer *pk = (CBasePlayer*) CBaseEntity::Instance( pKiller );

		if ( pk )
		{
			if ( (pk != pVictim) && (PlayerRelationship( pVictim, pk ) == GR_TEAMMATE) )
			{
				MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
					WRITE_BYTE( ENTINDEX(ENT(pKiller)) );		// the killer
					WRITE_BYTE( ENTINDEX(pVictim->edict()) );	// the victim
					WRITE_STRING( "teammate" );		// flag this as a teammate kill
				MESSAGE_END();
				return;
			}
		}
	}

	CHalfLifeMultiplay::DeathNotice( pVictim, pKiller, pevInflictor );
}

BOOL CRulesTeamDM :: FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	if ( pAttacker && PlayerRelationship( pPlayer, pAttacker ) == GR_TEAMMATE )
	{
		if ( (CVAR_GET_FLOAT("mp_friendlyfire") == 0) && (pAttacker != pPlayer) )
		{
			return FALSE;
		}
	}

	return CHalfLifeMultiplay::FPlayerCanTakeDamage( pPlayer, pAttacker );
}

const char *CRulesTeamDM :: GetIndexedTeamName( int teamIndex )
{
	switch( teamIndex )
	{
	case 1: return TEAM1;
		break;
	case 2: return TEAM2;
		break;
	default: return "";
		break;
	}
}

extern edict_t *EntSelectTeamSpawnPoint( CBaseEntity *pPlayer );

edict_t *CRulesTeamDM :: GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	edict_t *pentSpawnSpot = EntSelectTeamSpawnPoint( pPlayer );

	pPlayer->pev->origin = VARS(pentSpawnSpot)->origin + Vector(0,0,1);
	pPlayer->pev->v_angle  = g_vecZero;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->angles = VARS(pentSpawnSpot)->angles;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->pev->fixangle = TRUE;
	
	return pentSpawnSpot;
}

const char *CRulesTeamDM :: GetTeamID( CBaseEntity *pEntity )
{
	if ( pEntity == NULL || pEntity->pev == NULL )
		return "";

	// return their team name
	return pEntity->TeamID();
}

int CRulesTeamDM :: GetTeamIndex( const char *pTeamName )
{
	if ( pTeamName && *pTeamName != 0 )
	{
		if( FStrEq( pTeamName, TEAM1 ) )
			return 0;
		if( FStrEq( pTeamName, TEAM2 ) )
			return 1;
	}
	
	return -1;	// No match
}

extern int gmsgTeamNames;

void CRulesTeamDM :: InitHUD( CBasePlayer *pPlayer )
{
	pPlayer->m_iNextTeam = 0;		// unknown next team

	// populate the TFC team VGUI

	MESSAGE_BEGIN( MSG_ONE, gmsgTeamNames, NULL, pPlayer->edict() );
		WRITE_BYTE( 2 );
		WRITE_STRING( TEAM1 );
		WRITE_STRING( TEAM2 );
	MESSAGE_END();

	// start them off on a random team

	ChangePlayerTeam( pPlayer, TeamWithFewestPlayers( ), FALSE, FALSE );

	// run CHalfLifeMultiplay's InitHUD

	CHalfLifeMultiplay::InitHUD( pPlayer );

	// update this player with all the other players team info
	// loop through all active players and send their team info to the new client
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );
		if ( plr && IsValidTeam( plr->TeamID() ) )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgTeamInfo, NULL, pPlayer->edict() );
				WRITE_BYTE( plr->entindex() );
				WRITE_STRING( plr->TeamID() );
			MESSAGE_END();
		}
	}
}

int CRulesTeamDM :: IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	if ( !pKilled )
		return 0;

	if ( !pAttacker )
		return 1;

	if ( pAttacker != pKilled && PlayerRelationship( pAttacker, pKilled ) == GR_TEAMMATE )
		return -1;

	return 1;
}

void CRulesTeamDM :: PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	if ( !m_DisableDeathPenalty )
	{
		CHalfLifeMultiplay::PlayerKilled( pVictim, pKiller, pInflictor );
	}
}

int CRulesTeamDM :: PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	if ( !pPlayer || !pTarget || !pTarget->IsPlayer() )
		return GR_NOTTEAMMATE;

	CBasePlayer *pBPlayer = (CBasePlayer *)pPlayer;
	CBasePlayer *pBTarget = (CBasePlayer *)pTarget;

	if ( FStrEq( pBPlayer->m_szTeamName, pBTarget->m_szTeamName ) )
		return GR_TEAMMATE;

	return GR_NOTTEAMMATE;
}

int CRulesTeamDM :: PlayersOnTeam( const char *szTeamName )
{
	// returns # of players on a certain team

	int i;
	int iPlayers = 0;

	// loop through all players and increment for that team

	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex( i );

		if ( pEnt && pEnt->IsPlayer( ) )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pEnt;

			if ( FStrEq( pPlayer->m_szTeamName, szTeamName ) )
				iPlayers++;

		}
	}

	return iPlayers;
}

void CRulesTeamDM :: PlayerSpawn( CBasePlayer *pPlayer )
{
	pPlayer->pev->weapons |= (1<<WEAPON_SUIT);

	// change team if applicable

	if( pPlayer->m_iNextTeam > 0 )
	{
		switch( pPlayer->m_iNextTeam )
		{
		case 1: ChangePlayerTeam( pPlayer, TEAM1, FALSE, FALSE );	// join the team
			break;
		case 2: ChangePlayerTeam( pPlayer, TEAM2, FALSE, FALSE );
			break;
		case 5:	ChangePlayerTeam( pPlayer, TeamWithFewestPlayers( ), FALSE, FALSE );
		}

		pPlayer->m_iNextTeam = 0;		// don't change team next respawn
	}
}

BOOL CRulesTeamDM :: ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target )
{
	CBaseEntity *pTgt = CBaseEntity::Instance( target );

	if ( pTgt && pTgt->IsPlayer() )
	{
		if ( PlayerRelationship( pPlayer, pTgt ) == GR_TEAMMATE )
			return FALSE;	// don't autoaim at teammates
	}

	return CHalfLifeMultiplay::ShouldAutoAim( pPlayer, target );
}

int CRulesTeamDM :: TeamFrags( const char *szTeamName )
{
	// returns # of frags for a certain team

	int i;
	int iFrags = 0;

	// loop through all players and increment for that team

	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex( i );

		if ( pEnt && pEnt->IsPlayer( ) )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pEnt;

			if ( FStrEq( pPlayer->m_szTeamName, szTeamName ) )
				iFrags += pPlayer->pev->frags;
		}
	}

	return iFrags;
}

const char *CRulesTeamDM::TeamWithFewestPlayers( void )
{
	// returns team with fewest players

	if( PlayersOnTeam( TEAM1 ) < PlayersOnTeam( TEAM2 ) )
		return TEAM1;
	else if( PlayersOnTeam( TEAM1 ) > PlayersOnTeam( TEAM2 ) )
		return TEAM2;
	else	// returns random team if equal
	{
		if( RANDOM_LONG( 1, 2 ) == 1 )
			return TEAM1;
		else
			return TEAM2;
	}
}

extern DLL_GLOBAL BOOL g_fGameOver;
extern cvar_t timeleft;
extern cvar_t fragsleft;

void CRulesTeamDM :: Think ( void )
{
	static int last_frags;
	static int last_time;

	int frags_remaining = 0;
	int time_remaining = 0;

	//g_VoiceGameMgr.Update(gpGlobals->frametime);

	if ( g_fGameOver )	// someone else quit the game already
	{
		CHalfLifeMultiplay::Think( );
		return;
	}

	float flTimeLimit = CVAR_GET_FLOAT( "mp_timelimit" ) * 60;
	
	time_remaining = (int)(flTimeLimit ? (flTimeLimit - gpGlobals->time) : 0);

	if ( flTimeLimit != 0 && gpGlobals->time >= flTimeLimit )
	{
		GoToIntermission( );
		return;
	}

	float flFragLimit = fraglimit.value;
	if ( flFragLimit )
	{
		int bestfrags = 9999;
		int remain;

		// check if any team is over the frag limit

		if ( TeamFrags( TEAM1 ) >= flFragLimit || TeamFrags( TEAM2 ) >= flFragLimit )
		{
			GoToIntermission( );
			return;
		}

		if ( TeamFrags( TEAM1 ) >= TeamFrags( TEAM2 ) )
			remain = flFragLimit - TeamFrags( TEAM1 );
		else
			remain = flFragLimit - TeamFrags( TEAM2 );

		if ( remain < bestfrags )
		{
			bestfrags = remain;
		}
		frags_remaining = bestfrags;
	}

	if ( frags_remaining != last_frags )
	{
		g_engfuncs.pfnCvar_DirectSet( &fragsleft, UTIL_VarArgs( "%i", frags_remaining ) );
	}

	if ( timeleft.value != last_time )
	{
		g_engfuncs.pfnCvar_DirectSet( &timeleft, UTIL_VarArgs( "%i", time_remaining ) );
	}

	last_frags = frags_remaining;
	last_time  = time_remaining;
}

extern int gmsgGameMode;

void CRulesTeamDM :: UpdateGameMode( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgGameMode, NULL, pPlayer->edict() );
		WRITE_BYTE( 1 );	// game mode teamplay
	MESSAGE_END();
}
