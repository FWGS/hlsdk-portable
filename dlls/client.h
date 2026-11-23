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

#ifndef CLIENT_H
#define CLIENT_H

struct entity_state_s;
struct clientdata_s;
struct weapon_data_s;
struct usercmd_s;
struct netadr_s;
struct delta_s;

void		respawn( entvars_t* pev, BOOL copyCorpse );
BOOL		ClientConnect( edict_t* entity, const char* name, const char* address, char rejectReason[128] );
void		ClientDisconnect( edict_t* entity );
void		ClientKill( edict_t* entity );
void		ClientPutInServer( edict_t* entity );
void		ClientCommand( edict_t* entity );
void		ClientUserInfoChanged( edict_t* entity, char* infoBuffer );
void		ServerActivate( edict_t* edictList, int edictCount, int clientMax );
void		ServerDeactivate();
void		StartFrame();
void		PlayerPreThink( edict_t* entity );
void		PlayerPostThink( edict_t* entity );
void		ParmsNewLevel();
void		ParmsChangeLevel();
void		ClientPrecache();
const char*	GetGameDescription();
void		PlayerCustomization( edict_t* entity, customization_t* customization );
void		SpectatorConnect( edict_t* entity );
void		SpectatorDisconnect( edict_t* entity );
void		SpectatorThink( edict_t* entity );
void		Sys_Error( const char* error );
void		SetupVisibility( edict_t* viewEntity, edict_t* client, unsigned char** pvs, unsigned char** pas );
void		UpdateClientData( const edict_t* entity, int sendWeapons, clientdata_s* data );
int			AddToFullPack( entity_state_s* state, int index, edict_t* entity, edict_t* host,
				int hostFlags, int player, unsigned char* set );
void		CreateBaseline( int player, int index, entity_state_s* baseline, edict_t* entity,
				int playerModelIndex, Vector playerMins, Vector playerMaxs );
void		RegisterEncoders();
int			GetWeaponData( edict_t* player, weapon_data_s* data );
void		CmdStart( const edict_t* player, const usercmd_s* command, unsigned int seed );
void		CmdEnd( const edict_t* player );
int			ConnectionlessPacket( const netadr_s* from, const char* args, char* response, int* responseSize );
int			GetHullBounds( int hull, float* mins, float* maxs );
void		CreateInstancedBaselines();
int			InconsistentFile( const edict_t* player, const char* filename, char* disconnectMessage );
int			AllowLagCompensation();
void		Host_Say( edict_t* entity, int teamOnly );
void		ResetPlayerPVS( edict_t* client, int clientIndex );
void		MarkEntityInPVS( int clientIndex, int entityIndex, float time, bool inPVS );
bool		CheckEntityRecentlyInPVS( int clientIndex, int entityIndex, float time );
bool		CheckPlayerPVSLeafChanged( edict_t* client, int clientIndex );
extern "C" EXPORT void SV_SaveGameComment( char* buffer, int bufferSize );

void		LinkUserMessages();
void		UpdateEntitiesOnClientConnect( edict_t* player );
const char*	GetWeaponName( entvars_t* inflictor, entvars_t* killer );
edict_t*	EntSelectSpawnPoint( CBaseEntity* player );

#endif
