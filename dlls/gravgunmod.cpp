#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "gravgunmod.h"
#include "player.h"
#include "coop_util.h"


cvar_t cvar_allow_gravgun = { "mp_allow_gravgun","1", FCVAR_SERVER };
cvar_t cvar_allow_ar2 = { "mp_allow_ar2","0", FCVAR_SERVER };
cvar_t cvar_ar2_mp5 = { "mp_ar2_mp5","0", FCVAR_SERVER };
cvar_t cvar_ar2_balls = { "mp_ar2_balls","0", FCVAR_SERVER };
cvar_t cvar_ar2_bullets = { "mp_ar2_bullets","0", FCVAR_SERVER };
cvar_t cvar_wresptime = { "mp_wresptime","20", FCVAR_SERVER };
cvar_t cvar_iresptime = { "mp_iresptime","30", FCVAR_SERVER };
cvar_t cvar_gibtime = { "mp_gibtime","250", FCVAR_SERVER };
cvar_t cvar_hgibcount = { "mp_hgibcount","12", FCVAR_SERVER };
cvar_t cvar_agibcount = { "mp_agibcount","8", FCVAR_SERVER };
cvar_t mp_gravgun_players = { "mp_gravgun_players", "0", FCVAR_SERVER };

cvar_t mp_fixhornetbug = { "mp_fixhornetbug", "0", FCVAR_SERVER };
cvar_t mp_checkentities = { "mp_checkentities", "0", FCVAR_SERVER };

void Ent_RunGC_f( void );

void GGM_RegisterCVars( void )
{
	CVAR_REGISTER( &cvar_allow_ar2 );
	CVAR_REGISTER( &cvar_allow_gravgun );
	CVAR_REGISTER( &cvar_ar2_mp5 );
	CVAR_REGISTER( &cvar_ar2_bullets );
	CVAR_REGISTER( &cvar_ar2_balls );
	CVAR_REGISTER( &cvar_wresptime );
	CVAR_REGISTER( &cvar_iresptime );
	CVAR_REGISTER( &cvar_gibtime );
	CVAR_REGISTER( &cvar_hgibcount );
	CVAR_REGISTER( &cvar_agibcount );
	CVAR_REGISTER( &mp_gravgun_players );
	CVAR_REGISTER( &mp_fixhornetbug );
	CVAR_REGISTER( &mp_checkentities );
	g_engfuncs.pfnAddServerCommand( "ent_rungc", Ent_RunGC_f );
}

void Ent_RunGC( bool common, bool enttools, const char *userid )
{
	int i, count = 0, removed = 0;
	edict_t *ent = g_engfuncs.pfnPEntityOfEntIndex( gpGlobals->maxClients + 5 );

	ALERT( at_warning, "Running garbage collector\n" );

	for( i = gpGlobals->maxClients; i < gpGlobals->maxEntities; i++, ent++ )
	{
		const char *classname = STRING( ent->v.classname );

		if( ent->free )
			continue;

		if( !classname || !ent->v.classname || !classname[0] )
			continue;

		count++;

		if( ent->v.flags & FL_KILLME )
			continue;

		if( common )
		{
			if( !strcmp( classname, "gib" ) )
			{
				ent->v.flags |= FL_KILLME;
				removed++;
				continue;
			}

			if( !strncmp( classname, "monster_", 8 ) && ent->v.health <= 0 || ent->v.deadflag != DEAD_NO )
			{
				ent->v.flags |= FL_KILLME;
				removed++;
				continue;
			}
		}
		if( !enttools )
		{
			if( strncmp( classname, "monster_", 8 ) || strncmp( classname, "weapon_", 7 ) || strncmp( classname, "ammo_", 5 ) )
				continue;
		}

		if( !ent->v.owner && ent->v.spawnflags & SF_NORESPAWN )
		{
			ent->v.flags |= FL_KILLME;
			removed++;
			continue;
		}

		CBaseEntity *entity = CBaseEntity::Instance( ent );

		if( !entity )
		{
			ent->v.flags |= FL_KILLME;
			removed++;
			continue;
		}

		if( enttools && entity->enttools_data.enttools )
		{
			if( !userid || !strcmp( userid, entity->enttools_data.ownerid ) )
			{
				ent->v.flags |= FL_KILLME;
				removed++;
				continue;
			}
		}

		else if( common && !entity->IsInWorld() )
		{
			ent->v.flags |= FL_KILLME;
			removed++;
			continue;
		}
	}

	ALERT( at_notice, "Total %d entities, %d cleaned\n", count, removed );

}

void Ent_RunGC_f()
{
	int enttools = atoi(CMD_ARGV(1));
	Ent_RunGC( !enttools, enttools, NULL );
}

int Ent_CheckEntitySpawn( edict_t *pent )
{

	if( mp_checkentities.value )
	{
		int index = ENTINDEX( pent );
		static unsigned int counter, lastgc;
		counter++;


		if( gpGlobals->maxEntities - index < 10 )
		{
			ALERT( at_error, "REFUSING CREATING ENTITY %s\n", STRING( pent->v.classname ) );
			Ent_RunGC( true, true, NULL );
			return 1;
		}

		if( gpGlobals->maxEntities - index < 100 )
		{
			if( !strncmp( STRING(pent->v.classname), "env_", 4) )
				return 1;

			if( !strcmp( STRING(pent->v.classname), "gib" ) )
				return 1;


			Ent_RunGC( true, false, NULL );

			return 0;
		}

		if( index > gpGlobals->maxEntities / 2 && counter - lastgc > 64 )
		{
			lastgc = counter;
			Ent_RunGC( true, false, NULL );
			return 0;
		}
	}

	return 0;
}


void GGM_ClientPutinServer(edict_t *pEntity, CBasePlayer *pPlayer)
{
	if( mp_coop.value && pPlayer->gravgunmod_data.m_state == STATE_UNINITIALIZED )
		g_engfuncs.pfnQueryClientCvarValue2( pEntity, "touch_enable", 111 );

	pPlayer->gravgunmod_data.m_state = STATE_CONNECTED;

	const char *uid = GETPLAYERAUTHID( pPlayer->edict() );
	if( !uid || strstr(uid, "PENDING") )
		uid = g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "ip" );

	strncpy( pPlayer->gravgunmod_data.uid, uid, 32 );
	pPlayer->gravgunmod_data.uid[32] = 0;
	pPlayer->gravgunmod_data.m_flEntTime = 0;
	pPlayer->gravgunmod_data.m_flEntScope = 0;
}

void GGM_ClientFirstSpawn(CBasePlayer *pPlayer)
{
	// AGHL-like spectator
	if( mp_spectator.value )
	{
		pPlayer->RemoveAllItems( TRUE );
		UTIL_BecomeSpectator( pPlayer );
	}

}

edict_t *GGM_PlayerByID( const char *id )
{
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );

		if( plr && plr->IsPlayer() )
		{
			CBasePlayer *player = (CBasePlayer *) plr;

			if( !strcmp( player->gravgunmod_data.uid, id ) )
				return player->edict();
		}
	}

	return NULL;
}

const char *GGM_GetPlayerID( edict_t *player )
{
	CBasePlayer *plr = (CBasePlayer*)CBaseEntity::Instance( player );

	if( !plr->IsPlayer() )
		return NULL;

	return plr->gravgunmod_data.uid;
}
