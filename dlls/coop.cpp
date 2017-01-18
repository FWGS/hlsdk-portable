#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "coop_util.h"

void UTIL_CleanSpawnPoint( Vector origin, float dist )
{
	CBaseEntity *ent = NULL;
	while( ( ent = UTIL_FindEntityInSphere( ent, origin, dist ) ) != NULL )
	{
		if( ent->IsPlayer() )
		{
			TraceResult tr;
			UTIL_TraceHull( ent->pev->origin + Vector( 0, 0, 36), ent->pev->origin + Vector( RANDOM_FLOAT( -150, 150 ), RANDOM_FLOAT( -150, 150 ), 0 ), dont_ignore_monsters, human_hull, ent->edict(), &tr);
			//UTIL_TraceModel( ent->pev->origin + Vector( 0, 0, 36), ent->pev->origin + Vector( RANDOM_FLOAT( -150, 150 ), RANDOM_FLOAT( -150, 150 ), 0 ), 0, ent->edict(), &tr);
			if( !tr.fAllSolid )
				UTIL_SetOrigin(ent->pev, tr.vecEndPos);
		}
	}
}


Vector UTIL_FixupSpawnPoint(Vector spawn)
{
	int i = 0;
	// predict that spawn point is almost correct
	while( i < 2 ) // 2 player heights
	{
		Vector point = spawn + Vector( 0, 0, 36 * i );
		TraceResult tr;
		UTIL_TraceHull( point, point, ignore_monsters, (mp_unduck.value&&g_fSavedDuck)?head_hull:human_hull, NULL, &tr );
		if( !tr.fStartSolid && !tr.fAllSolid )
			return point;
		i = -i;
		if( i >= 0 )
			i++;
	}
	return spawn;
}

void UTIL_CoopSaveTrain( CBaseEntity *pPlayer, SavedCoords *coords)
{
	if( coords->trainsaved )
		return;

	CBaseEntity *train = UTIL_CoopGetPlayerTrain(pPlayer);
	if( !train )
	{
		ALERT( at_console, "^1NO TRAIN!\n");
		return;
	}
	ALERT( at_console, "^1TRAIN IS %s\n", STRING( train->pev->classname ) );

	if( !pPlayer->IsPlayer() )
	{
		// it is trainnitself, try find player on it
		CBaseEntity *pList;
		Vector mins = pPlayer->pev->absmin;
		Vector maxs = pPlayer->pev->absmax;
		maxs.z += 72;
		int count = UTIL_EntitiesInBox( &pList, 1, mins, maxs, FL_ONGROUND );
		if( count && pList && pList->IsPlayer() )
			pPlayer = pList;
		else
		{
			ALERT( at_console, "Train without players\n" );
			return;
		}
	}

	strcpy( coords->trainglobal, STRING(train->pev->globalname) );
	coords->trainoffset = pPlayer->pev->origin - VecBModelOrigin(train->pev);
	coords->trainsaved = true;
}

void UTIL_BecomeSpectator( CBasePlayer *pPlayer )
{
	//pPlayer->m_bDoneFirstSpawn = true;
	pPlayer->pev->takedamage = DAMAGE_NO;
	pPlayer->pev->flags |= FL_SPECTATOR;
	pPlayer->pev->flags |= FL_NOTARGET;
	pPlayer->pev->effects |= EF_NODRAW;
	pPlayer->pev->solid = SOLID_NOT;
	pPlayer->pev->movetype = MOVETYPE_NOCLIP;
	pPlayer->pev->modelindex = 0;
	pPlayer->pev->health = 1;
	pPlayer->m_pGoalEnt = NULL;
	return;
}

void UTIL_SpawnPlayer( CBasePlayer *pPlayer )
{
	pPlayer->m_state = STATE_SPAWNED;
	pPlayer->m_iRespawnFrames = 0;
	pPlayer->pev->effects &= ~EF_NODRAW;

	pPlayer->pev->takedamage = DAMAGE_YES;
	pPlayer->pev->flags &= ~FL_SPECTATOR;
	pPlayer->pev->movetype = MOVETYPE_WALK;
	pPlayer->Spawn();
	CLIENT_COMMAND( pPlayer->edict(), "touch_show _coopm*\n" );

}

char * UTIL_CoopPlayerName( CBaseEntity *pPlayer )
{
	if( !pPlayer )
		return (char*)"unnamed(NULL)";
	return (char*)( ( pPlayer->pev->netname && STRING( pPlayer->pev->netname )[0] != 0 ) ? STRING( pPlayer->pev->netname ) : "unconnected" );
}

bool g_fSavedDuck;

char *badlist[256] = {
"player", // does not even can set own name
"talat",
"hmse",
"mhmd",
"aeman",
"famas",
"danek",
"ame syia",
"melih",
"aliance",
"vladick"
};

void UTIL_CoopKickPlayer(CBaseEntity *pPlayer)
{
	int i;
	if( !pPlayer )
		return;
	char *name = UTIL_CoopPlayerName( pPlayer );
	SERVER_COMMAND( UTIL_VarArgs( "kick %d\n", ENTINDEX(pPlayer->pev->pContainingEntity) - 1 ) );
	if( strlen( name ) < 5 )
		return;
	for( i = 0; badlist[i]; i++ );
	badlist[i] = strdup( name );
}

bool UTIL_CoopIsBadPlayer( CBaseEntity *plr )
{
	if( !plr )
		return false;
	for( int i = 0; badlist[i];i++ )
		if( strcasestr( (char*)UTIL_CoopPlayerName( plr ), badlist[i] ) )
			return true;
	return false;
}

struct checkpoint_s
{
	char str[32];
	float time;
	Vector origin;
	Vector angles;
} g_checkpoints[4];


void CoopClearData( void )
{
	// nullify
	SavedCoords l_SavedCoords = {0};
	g_SavedCoords = l_SavedCoords;
	memset( &g_checkpoints, 0, sizeof( g_checkpoints ) );
}

bool g_fPause;
void CoopApplyData( void )
{
	if( s_SavedCoords.valid )
	{
		struct SavedCoords null1 = {0};
		g_SavedCoords = s_SavedCoords;
		s_SavedCoords = null1;
		g_fSavedDuck = g_SavedCoords.fDuck;
	}
	g_fPause = false;
	ALERT( at_console, "^2CoopApplyData()\n" );
	memset( &g_checkpoints, 0, sizeof( g_checkpoints ) );
}


int g_iMenu;


void UTIL_CoopLocalConfirmMenu(CBasePlayer *pPlayer)
{
		const char *menu[] = {
			"No",
			"Cancel",
			"Do not confirm",
			"Don't confirm",
			"Единая Россия"
		};

		menu[pPlayer->m_iConfirmKey = RANDOM_LONG(2,4)] = "Confirm";
		UTIL_CoopShowMenu(pPlayer, "Confirm changing map BACK (NOT RECOMMENDED)?", ARRAYSIZE(menu), menu);
		pPlayer->m_iMenuState = MENUSTATE_LOCAL_CONFIRM;
}


void GlobalMenu::Process( CBasePlayer *pPlayer, int imenu )
{
	if( pPlayer->pev->flags & FL_SPECTATOR )
		return;
	if( gpGlobals->time - m_flTime > 30 )
	{
		g_iMenu = 0;
		return;
	}

	switch( g_iMenu )
	{
	case 1: // touch blue trigger
		m_iVoteCount++;

		if( imenu == 1 ) // confirm
		{
		if( m_iBanCount >= 2 )
		{
			UTIL_CoopKickPlayer( pPlayer );
			m_iConfirm-= 5;
			return;
		}				m_iConfirm++;
		MESSAGE_BEGIN( MSG_ALL, 8, NULL ); // svc_print
			WRITE_BYTE( 3 ); // PRINT_CHAT
			WRITE_STRING( UTIL_VarArgs( "%s^7 confirmed map change\n", ( pPlayer->pev->netname && STRING( pPlayer->pev->netname )[0] != 0 ) ? STRING( pPlayer->pev->netname ) : "unconnected"));
		MESSAGE_END();

		}
		if( imenu == 2 ) // cancel
		{
			m_iConfirm--;
			if( pPlayer == m_pPlayer )
				m_iConfirm -= 100; // player mistake
		}
		if( imenu == 3 )
		{
			m_iBanCount++;
			if( m_iBanCount >= 2 && m_iConfirm > -50 )
				UTIL_CoopKickPlayer( m_pPlayer );
		}
		break;
	case 2: // vote by request
		MESSAGE_BEGIN( MSG_ALL, 8, NULL ); // svc_print
			WRITE_BYTE( 3 ); // PRINT_CHAT
			WRITE_STRING( UTIL_VarArgs( "%s^7 selected ^3%s\n", ( pPlayer->pev->netname && STRING( pPlayer->pev->netname )[0] != 0 ) ? STRING( pPlayer->pev->netname ) : "unconnected", maps[imenu - 1] ));
		MESSAGE_END();

		if( imenu < m_iConfirm )
		{
			votes[imenu-1]++;
			m_iVoteCount++;

			if( votes[1] >= 2 )
			{
				 // two players vote for ban
				UTIL_CoopKickPlayer( m_pPlayer );
			}

			if( m_iVoteCount >= m_iMaxCount )
			{
				for( int i = 0; i <= m_iConfirm; i++ )
					if( votes[i] >= m_iMaxCount )
					{
						UTIL_CoopActivateChangeLevel( triggers[i] );
						g_iMenu = 0;
					}

			}

		}
	}
}
void GlobalMenu::ShowGlobalMenu( const char *title, int count, const char **menu )
{
	int count2 = 0;
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );

		if( plr && plr->IsPlayer() && !UTIL_CoopIsBadPlayer( plr ) )
		{
			count2++;
			CBasePlayer *player = (CBasePlayer *) plr;
			UTIL_CoopShowMenu( player, title, count, menu, 30 );
			player->m_iMenuState = MENUSTATE_GLOBAL;

		}
	}
	m_iMaxCount = count2;
	m_iBanCount = 0;
}

void GlobalMenu::ConfirmMenu( CBasePlayer *pPlayer, CBaseEntity *trigger, const char *mapname )
{
	if( g_iMenu && gpGlobals->time - m_flTime < 30 )
		return; // wait 30s befor new confirm vote
	if( pPlayer->m_iMenuState == MENUSTATE_LOCAL_CONFIRM )
		return;
	if( pPlayer->m_iLocalConfirm < 3 )
	{
		UTIL_CoopLocalConfirmMenu( pPlayer );
		return;
	}
	g_iMenu = 1;
	m_flTime = gpGlobals->time;
	m_pTrigger = trigger;
	m_pPlayer = pPlayer;
	const char *menu[] = {
		"Confirm",
		"Cancel",
		"BAN"
	};
		MESSAGE_BEGIN( MSG_ALL, 8, NULL ); // svc_print
			WRITE_BYTE( 3 ); // PRINT_CHAT
			WRITE_STRING( UTIL_VarArgs( "%s^7 wants to change map ^1BACKWARDS\n", ( pPlayer->pev->netname && STRING( pPlayer->pev->netname )[0] != 0 ) ? STRING( pPlayer->pev->netname ) : "unconnected"));
		MESSAGE_END();
	ShowGlobalMenu(UTIL_VarArgs("Confirm changing map BACK TO %s?", mapname), ARRAYSIZE(menu), menu);

}



void UTIL_CoopCheckpointMenu( CBasePlayer *pPlayer )
{
	//if( pPlayer->m_state == STATE_SPAWNED )
	{

		if( mp_coop_checkpoints.value )
		{
			const char *menu[5] = {
				"New checkpoint"
			};
			int i;
			if( pPlayer->m_state == STATE_SPECTATOR || pPlayer->m_state == STATE_SPECTATOR_BEGIN )
				menu[0] = "Just spawn";
			for( i = 1; g_checkpoints[i-1].time; i++ )
				menu[i] = g_checkpoints[i-1].str;
			UTIL_CoopShowMenu( pPlayer, "Select checkpoint", i, menu );
			pPlayer->m_iMenuState = MENUSTATE_CHECKPOINT;
		}
	}
}


void UTIL_CoopNewCheckpoint( entvars_t *pevPlayer )
{
	memmove( &g_checkpoints[1], &g_checkpoints[0], sizeof ( g_checkpoints[0] ) * 3 );
	g_checkpoints[0].time = gpGlobals->time;
	snprintf( g_checkpoints[0].str, 31,  "%5s %d", STRING( pevPlayer->netname ), (int)( gpGlobals->time / 60 ) );
	g_checkpoints[0].origin = pevPlayer->origin;
	g_checkpoints[0].angles = pevPlayer->angles;
			MESSAGE_BEGIN( MSG_ALL, 8, NULL ); // svc_print
				WRITE_BYTE( 3 ); // PRINT_CHAT
				WRITE_STRING( "New checkpoint availiable\n" );
			MESSAGE_END();

}


void UTIL_CoopVoteMenu( CBasePlayer *pPlayer )
{
	int count = 0;
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );

		if( plr && plr->IsPlayer() )
		{
			count++;
		}
	}
	if( count < 4 )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Need at least 4 players to vote changelevel!\n" );
		return;
	}
	g_GlobalMenu.VoteMenu(pPlayer);
}

void UTIL_CoopMenu( CBasePlayer *pPlayer )
{
	if( pPlayer->m_state == STATE_SPAWNED )
	{
		pPlayer->m_iMenuState = MENUSTATE_COOPMENU;
		if( mp_coop.value )
		{
			const char *menu[] = {
				"Force respawn",
				"Unblock",
				"Become spectator",
				"Vote changelevel",
				"Checkpoint/restore"
			};
			int count1 = ARRAYSIZE( menu ) - 1;
			if( mp_coop_checkpoints.value )
				count1++;
			UTIL_CoopShowMenu( pPlayer, "Coop menu", count1, menu );
		}
	}
	else if ( pPlayer->m_state == STATE_SPECTATOR )
	{
		pPlayer->m_iMenuState = MENUSTATE_COOPMENU_SPEC;
		if( mp_coop.value )
		{
			const char *menu[] = {
				"Spawn",
				"Close menu"
			};
			UTIL_CoopShowMenu( pPlayer, "Spectator menu", ARRAYSIZE( menu ), menu );
		}
	}
}


void UTIL_CoopProcessMenu( CBasePlayer *pPlayer, int imenu )
{
			switch( pPlayer->m_iMenuState )
			{
				case MENUSTATE_COOPMENU_SPEC:
					if( imenu == 1 )
					{
						if( g_checkpoints[0].time )
							UTIL_CoopCheckpointMenu( pPlayer );
						else
						{
							UTIL_SpawnPlayer( pPlayer );
							pPlayer->m_state = STATE_SPAWNED;
						}
					}
					if( imenu == 2 )
					{
						pPlayer->m_state = STATE_SPECTATOR;
						CLIENT_COMMAND( pPlayer->edict(), "touch_show _coopm*\n" );
					}
				break;
				case MENUSTATE_COOPMENU:
					if( pPlayer->m_state != STATE_SPAWNED )
						break;
					if( imenu == 1 )
					{
						pPlayer->RemoveAllItems( TRUE );
						UTIL_SpawnPlayer( pPlayer );
					}
					if( imenu == 2 )
					{
						UTIL_CleanSpawnPoint( pPlayer->pev->origin, 150 );
					}
					if( imenu == 3 )
					{
						pPlayer->RemoveAllItems( TRUE );
						UTIL_BecomeSpectator( pPlayer );
						pPlayer->m_state = STATE_SPECTATOR;
					}
					if( imenu == 4 )
					{
						UTIL_CoopVoteMenu( pPlayer );
					}
					if( imenu == 5 )
					{
						UTIL_CoopCheckpointMenu( pPlayer );
					}
				break;
				case MENUSTATE_GLOBAL:
					if( !UTIL_CoopIsBadPlayer( pPlayer ) )
						g_GlobalMenu.Process( pPlayer, imenu );
				break;
				case MENUSTATE_CHECKPOINT:
					if( imenu == 1 )
					{
						if( pPlayer->m_state == STATE_SPECTATOR_BEGIN )
							UTIL_SpawnPlayer( pPlayer );
						else if( !UTIL_CoopIsBadPlayer( pPlayer ) )
							UTIL_CoopNewCheckpoint( pPlayer->pev );
					}
					else if( imenu > 1 && imenu < 5 )
					{
						pPlayer->RemoveAllItems( TRUE );
						UTIL_SpawnPlayer( pPlayer );
						pPlayer->pev->origin = g_checkpoints[imenu-2].origin;
					}
				break;
				case MENUSTATE_LOCAL_CONFIRM:
					if( imenu - 1 == pPlayer->m_iConfirmKey )
						pPlayer->m_iLocalConfirm++;
					else
						pPlayer->m_iLocalConfirm = 0;
					pPlayer->m_iMenuState = MENUSTATE_NONE;
				break;
				default:
				break;
			}
	//pPlayer->m_iMenuState = MENUSTATE_NONE;
}

bool UTIL_CoopRestorePlayerCoords(CBaseEntity *player, Vector *origin, Vector *angles )
{
	if(!g_SavedCoords.valid)
		return false;
	UTIL_CoopValidateOffset();
	// compute player by IQ
	char *ip = g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( player->edict() ), "ip" );
	for( int i = 0;i < g_SavedCoords.iCount;i++)
	{
		if(ip && ip[0] && !strcmp(ip, g_SavedCoords.ip[i]) )
		{
			TraceResult tr;
			Vector point = g_SavedCoords.origin[i] + g_SavedCoords.offset;

			UTIL_TraceHull( point, point, missile, (mp_unduck.value&&g_fSavedDuck)?head_hull:human_hull, NULL, &tr );

			g_SavedCoords.ip[i][0] = 0;

			if( tr.fStartSolid || tr.fAllSolid )
				return false;
			*origin = point;
			*angles = g_SavedCoords.angles[i];
			return true;
		}
	}
	return false;
}

bool UTIL_CoopGetSpawnPoint( Vector *origin, Vector *angles)
{
	if(!g_SavedCoords.valid)
		return false;

	// spawn on elevator or train
	if( g_SavedCoords.trainsaved )
	{
		CBaseEntity *train = UTIL_FindEntityByString( NULL, "globalname", g_SavedCoords.trainglobal );
		if( !train ) train = UTIL_FindEntityByString( NULL, "classname", g_SavedCoords.trainglobal );
		if( train && ( !g_SavedCoords.trainuser1 || train->pev->iuser1 == g_SavedCoords.trainuser1 ) )
		{
			*angles = g_SavedCoords.triggerangles;
			*origin = VecBModelOrigin(train->pev) + g_SavedCoords.trainoffset;
			g_SavedCoords.trainuser1 = train->pev->iuser1;
			return true;
		}
		ALERT( at_console, "Failed to get train %s (map design error?)\n", g_SavedCoords.trainglobal );
	}
	Vector point = g_SavedCoords.triggerorigin;
	*angles = g_SavedCoords.triggerangles;
	if( !g_SavedCoords.validspawnpoint )
	{
		TraceResult tr;
		Vector angle;
		UTIL_MakeVectorsPrivate( *angles, (float*)&angle, NULL, NULL );
		UTIL_CoopValidateOffset();
		point = point + g_SavedCoords.offset;
		//UTIL_TraceHull( point, point, ignore_monsters, human_hull, NULL, &tr );

		if( mp_unduck.value && g_fSavedDuck && !g_SavedCoords.fUsed )
			UTIL_TraceHull( point, point + angle * 100, missile, head_hull, NULL, &tr );
		else
			UTIL_TraceHull( point, point + angle * 100, ignore_monsters, human_hull, NULL, &tr );

		if( !tr.fStartSolid && !tr.fAllSolid || ENTINDEX( tr.pHit ) && ENTINDEX( tr.pHit ) <= gpGlobals->maxClients )
		{
			//g_SavedCoords.triggerorigin = tr.vecEndPos;
			//g_SavedCoords.validspawnpoint = true;
			if( tr.pHit && FClassnameIs( tr.pHit, "func_door" ) )
				tr.pHit->v.solid = SOLID_NOT;
			ALERT( at_console, "CoopGetSpawnPoint: ^2offset set\n");

		}
		else
		{
			//g_SavedCoords.valid = false;
			ALERT( at_console, "CoopGetSpawnPoint: ^2trace failed\n");
			return false;
		}
	}
	*origin = point;

	return true;
}


CBaseEntity *UTIL_CoopGetPlayerTrain( CBaseEntity *pPlayer)
{
	CBaseEntity *train = NULL;

	if( !pPlayer)
		return NULL;

	if( !pPlayer->IsPlayer() )
	{
		// activated by path track
		train = pPlayer;
	}
	else
	{
		if( FNullEnt(pPlayer->pev->groundentity))
			return NULL;

		train = CBaseEntity::Instance(pPlayer->pev->groundentity);
	}

	if( !train )
		return NULL;
	if( !train->pev->globalname ||!STRING(train->pev->globalname) || !STRING(train->pev->globalname)[0] )
		return NULL;
	// doors are elevators
	if( strcmp( STRING( train->pev->classname ), "func_train") && strcmp( STRING( train->pev->classname ), "func_tracktrain") && strcmp( STRING( train->pev->classname ), "func_door")   )
		return NULL;
	return train;
}

GlobalMenu g_GlobalMenu;


struct SavedCoords g_SavedCoords, s_SavedCoords;
