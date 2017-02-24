#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "coop_util.h""

GlobalMenu g_GlobalMenu;


struct SavedCoords g_SavedCoords, s_SavedCoords;

static float msglimittime1, msglimittime2;


void UTIL_CoopPlayerMessage( CBaseEntity *pPlayer, int channel, float time, unsigned int color1, unsigned int color2, float x, float y,  const char *format, ... )
{
	if( !pPlayer )
		return;

	hudtextparms_t params;
	params.x = x, params.y = y;
	params.fadeinTime = params.fadeoutTime = .5f;
	params.holdTime = time;
	params.r1 = (color1 >> 24) & 0xFF, params.g1 = (color1 >> 16) & 0xFF, params.b1 = (color1 >> 8) & 0xFF, params.a1 = color1 & 0xFF;
	params.r2 = (color2 >> 24) & 0xFF, params.g2 = (color2 >> 16) & 0xFF, params.b2 = (color2 >> 8) & 0xFF, params.a2 = color2 & 0xFF;
	params.channel = channel;
	va_list	argptr;
	char string[256];

	va_start( argptr, format );
	int len = vsnprintf( string, 256, format, argptr );
	va_end( argptr );
	string[len] = 0;
	char *pstr = string;

	// set line breaks
	for( int i = 0; *pstr; pstr++,i++ )
	{
		if( *pstr == '\n' )
			i = 0;
		if( i >= 79 )
			*pstr = '\n', i = 0;
	}

	UTIL_HudMessage( pPlayer, params, string );
}

void UTIL_CoopHudMessage( int channel, float time, unsigned int color1, unsigned int color2, float x, float y,  const char *format, ... )
{
	if( gpGlobals->time < msglimittime1 )
		return;
	msglimittime1 = gpGlobals->time + 0.4;

	hudtextparms_t params;
	params.x = x, params.y = y;
	params.fadeinTime = params.fadeoutTime = .5f;
	params.holdTime = time;
	params.r1 = (color1 >> 24) & 0xFF, params.g1 = (color1 >> 16) & 0xFF, params.b1 = (color1 >> 8) & 0xFF, params.a1 = color1 & 0xFF;
	params.r2 = (color2 >> 24) & 0xFF, params.g2 = (color2 >> 16) & 0xFF, params.b2 = (color2 >> 8) & 0xFF, params.a2 = color2 & 0xFF;
	params.channel = channel;
	va_list	argptr;
	char string[256];

	va_start( argptr, format );
	int len = vsnprintf( string, 256, format, argptr );
	va_end( argptr );
	string[len] = 0;
	char *pstr = string;

	// set line breaks
	for( int i = 0; *pstr; pstr++,i++ )
	{
		if( *pstr == '\n' )
			i = 0;
		if( i >= 79 )
			*pstr = '\n', i = 0;
	}

	UTIL_HudMessageAll( params, string );
}

void UTIL_CoopPrintMessage( const char *format, ... )
{
	if( gpGlobals->time < msglimittime2 )
		return;
	msglimittime2 = gpGlobals->time + 0.4;

	va_list	argptr;
	char string[256];

	va_start( argptr, format );
	int len = vsnprintf( string, 256, format, argptr );
	va_end( argptr );
	string[len] = 0;

	MESSAGE_BEGIN( MSG_ALL, 8, NULL ); // svc_print
		WRITE_BYTE( 3 ); // PRINT_CHAT
		WRITE_STRING( string );
	MESSAGE_END();
}


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

	// find last slot
	for( i = 0; badlist[i]; i++ );

	badlist[i] = strdup( name );
}
#ifdef __WIN32 // no strcasestr
#include <windows.h>
#include <string.h>
const char *strcasestr( const char *s1, const char *s2 )
{
	if( s1 == 0 || s2 == 0 )
		return 0;

	size_t n = strlen(s2);

	while(*s1)
		if(!strnicmp(s1++,s2,n))
			return (s1-1);

	return 0;
}
#endif

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


void UTIL_CoopClearData( void )
{
	// nullify
	SavedCoords l_SavedCoords = {0};
	g_SavedCoords = l_SavedCoords;
	memset( &g_checkpoints, 0, sizeof( g_checkpoints ) );
	msglimittime1 = msglimittime2 = 0;
}

bool g_fPause;
void UTIL_CoopApplyData( void )
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
			}
			m_iConfirm++;
			UTIL_CoopPrintMessage( "%s^7 confirmed map change\n", UTIL_CoopPlayerName( pPlayer ));

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
		UTIL_CoopPrintMessage( "%s^7 selected ^3%s\n", UTIL_CoopPlayerName( pPlayer ), maps[imenu - 1] );

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

	UTIL_CoopPrintMessage( "%s^7 wants to change map ^1BACK to %s\n", UTIL_CoopPlayerName( pPlayer ), mapname );
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
	if( !pevPlayer->netname )
		return;
	memmove( &g_checkpoints[1], &g_checkpoints[0], sizeof ( g_checkpoints[0] ) * 3 );
	g_checkpoints[0].time = gpGlobals->time;
	snprintf( g_checkpoints[0].str, 31,  "%5s %d", STRING( pevPlayer->netname ), (int)( gpGlobals->time / 60 ) );
	g_checkpoints[0].origin = pevPlayer->origin;
	g_checkpoints[0].angles = pevPlayer->angles;
	UTIL_CoopPrintMessage( "New checkpoint by %s!\n", STRING( pevPlayer->netname ) );

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
		UTIL_CoopPlayerMessage( pPlayer, 0, 5, 0xFFFFFFFF, 0xFFFFFFFF, -1, -1, "Need at least 4 players to vote changelevel!\n");
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
						if( pPlayer->m_state != STATE_SPAWNED )
							UTIL_SpawnPlayer( pPlayer );
						else if( !UTIL_CoopIsBadPlayer( pPlayer ) )
							UTIL_CoopNewCheckpoint( pPlayer->pev );
					}
					else if( imenu > 1 && imenu < 5 )
					{
						pPlayer->RemoveAllItems( TRUE );
						UTIL_SpawnPlayer( pPlayer );
						pPlayer->pev->origin = g_checkpoints[imenu-2].origin;
						pPlayer->pev->angles = g_checkpoints[imenu-2].angles;
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



// Collect all weapons tat player touchet in coop ant give to all players at spawn

CWeaponList g_WeaponList;

void CWeaponList::Clear()
{
	m_iWeapons = 0;
}
void CWeaponList::GiveToPlayer(CBasePlayer *pPlayer)
{
	for(int i = 0; i < m_iWeapons;i++)
		pPlayer->GiveNamedItem(weapons[i]);
}
void CWeaponList::AddWeapon( const char *classname )
{
	int i;
	for(i = 0; i < m_iWeapons;i++)
		if(!strcmp(weapons[i], classname))
			return;
	strcpy(weapons[m_iWeapons++], classname);
}
extern int gmsgShowMenu;

void UTIL_CoopShowMenu( CBasePlayer *pPlayer, const char *title, int count, const char **slot, signed char time )
{
	if( pPlayer->m_fTouchMenu)
	{
		char buf[256];
		#define MENU_STR(VAR) (#VAR)
		sprintf( buf, MENU_STR(slot10\ntouch_hide _coops*\ntouch_show _coops\ntouch_addbutton "_coopst" "#%s" "" 0.16 0.11 0.41 0.3 0 255 0 255 78 1.5\n), title);
		CLIENT_COMMAND( pPlayer->edict(), buf);
		for( int i = 0; i < count; i++ )
		{
			sprintf( buf, MENU_STR(touch_settexture _coops%d "#%d. %s"\ntouch_show _coops%d\n), i+1, i+1, slot[i], i + 1 );
			CLIENT_COMMAND( pPlayer->edict(), buf);
		}
	}
	else
	{
		char buf[128], *pbuf = buf;
		short int flags = 1<<9;
		pbuf += sprintf( pbuf, "^2%s:\n", title );
		for( int i = 0; i < count; i++ )
		{
			pbuf += sprintf( pbuf, "^3%d.^7 %s\n", i+1, slot[i]);
			flags |= 1<<i;
		}
		MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, NULL, pPlayer->pev);
		WRITE_SHORT( flags ); // slots
		WRITE_CHAR( time ); // show time
		WRITE_BYTE( 0 ); // need more
		WRITE_STRING( buf );
		MESSAGE_END();
	}
	//CLIENT_COMMAND( pPlayer->edict(), "exec touch_default/numbers.cfg\n");
}

bool UTIL_CoopConfirmMenu(CBaseEntity *pTrigger, CBaseEntity *pActivator, int count2, char *mapname )
{
	if( gpGlobals->time - g_GlobalMenu.m_flTime > 30 )
	{
		g_iMenu = 0;
		g_GlobalMenu.m_iConfirm = 0;
	}
	if( g_iMenu != 1 )
	{
		if( !UTIL_CoopIsBadPlayer( pActivator ) )
			g_GlobalMenu.ConfirmMenu( (CBasePlayer*)pActivator, pTrigger, mapname );
		return false;
	}
	if( g_GlobalMenu.m_iConfirm < count2 )
		return false;
	if( mp_coop_strongcheckpoints.value )
	{
		// do not allow go back if there are checkpoints, but not near changelevel
		if( g_checkpoints[0].time && (g_checkpoints[0].origin - VecBModelOrigin(pTrigger->pev)).Length() > 150 )
		{
			g_GlobalMenu.m_iConfirm = 0;
			return false;
		}
		//if( count2 < 2 )
			//return;
	}
	return true;
}


#include <string.h>
/* @NOPEDANTRY: ignore use of reserved identifier */
static char *strrstr(const char *x, const char *y) {
char *prev = NULL;
char *next;
if (*y == '\0')
return (char*)strchr(x, '\0');
while ((next = (char*)strstr(x, y)) != NULL) {
prev = next;
x = next + 1;
}
return prev;
}


int UTIL_CheckForEntTools( edict_t *pent )
{
	if( pent->v.targetname )
	{
		char *s = (char*)STRING( pent->v.targetname );
		char str[256];
		strcpy( str, s );
		s = strrstr( str, "_e" );
		if( s )
		{
			*s = 0;
			s = s + 2;
			if( atoi(s) == ENTINDEX( pent ) )
			{
				s = strrstr( str, "_");
				if( s )
				{
					int userid = atoi( s + 1 );
					for( int i = 1; i < gpGlobals->maxClients; i++ )
						if(  userid == g_engfuncs.pfnGetPlayerUserId( INDEXENT( i ) ) )
							 return i;
				}
			}
		}
	}
	return 0;
}

int UTIL_CoopCheckSpawn( edict_t *pent )
{
	if( mp_checkentities.value )
	{
		const char *szClassName = NULL;

		if( !pent->v.classname )
			return 0;

		szClassName = STRING( pent->v.classname );

		if( !szClassName || !szClassName[0] )
			return 0;

		if( strstr( szClassName, "monster_") )
		{
			CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( UTIL_CheckForEntTools( pent ) );

			if( pPlayer )
			{
				if( UTIL_CoopIsBadPlayer( pPlayer ) )
				{
					pent->v.flags = FL_KILLME;
					return -1;
				}

				if( gpGlobals->time - pPlayer->m_fEnttoolsMonsterTime < 5 )
				{
					UTIL_CoopKickPlayer( pPlayer );
					pent->v.flags = FL_KILLME;
					return -1;
				}

				if( gpGlobals->time - pPlayer->m_fEnttoolsMonsterTime > 120 )
					pPlayer->m_iEnttoolsMonsters = 0;

				if( pPlayer->m_iEnttoolsMonsters > 5 )
				{
					UTIL_CoopKickPlayer( pPlayer );
					pent->v.flags = FL_KILLME;
					return -1;
				}
				pPlayer->m_iEnttoolsMonsters++;
				pPlayer->m_fEnttoolsMonsterTime = gpGlobals->time;
			}

		}
	}
	return 0;
}
