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
#pragma once
#ifndef THREEWAVE_GAMERULES_H
#define THREEWAVE_GAMERULES_H

#define BLUE 2
#define RED 1

#if !NO_VOICEGAMEMGR
#include "voice_gamemgr.h"
#endif

extern int gmsgCTFMsgs;
extern int gmsgShowMenu;
extern int gmsgFlagStatus;
extern int gmsgRuneStatus;
extern int gmsgFlagCarrier;
extern int gmsgScoreInfo;

#define MAX_TEAMNAME_LENGTH	16
#define MAX_TEAMS			32

#define TEAMPLAY_TEAMLISTLENGTH		MAX_TEAMS*MAX_TEAMNAME_LENGTH

class CThreeWave : public CHalfLifeMultiplay
{
public:
	CThreeWave();

	virtual BOOL ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] );
	virtual BOOL ClientCommand( CBasePlayer *pPlayer, const char *pcmd );
	virtual void ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer );
	virtual BOOL IsTeamplay( void );
	virtual BOOL FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker );
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );
	virtual const char *GetTeamID( CBaseEntity *pEntity );
	virtual BOOL ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target );
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void InitHUD( CBasePlayer *pl );
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor );
	virtual const char *GetGameDescription( void ) { return "3Wave CTF"; }  // this is the game name that gets seen in the server browser
	virtual void UpdateGameMode( CBasePlayer *pPlayer );  // the client needs to be informed of the current game mode
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual void Think ( void );
	virtual int GetTeamIndex( const char *pTeamName );
	virtual const char *GetIndexedTeamName( int teamIndex );
	virtual BOOL IsValidTeam( const char *pTeamName );
	virtual void ChangePlayerTeam( CBasePlayer *pPlayer, int iTeam );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	void JoinTeam ( CBasePlayer *pPlayer, int iTeam );
	int TeamWithFewestPlayers ( void );
	virtual void ClientDisconnected( edict_t *pClient );
	void GetFlagStatus( CBasePlayer *pPlayer );

	virtual edict_t *GetPlayerSpawnSpot( CBasePlayer *pPlayer );

	virtual void PlayerThink( CBasePlayer *pPlayer );

	void PlayerTakeDamage( CBasePlayer *pPlayer , CBaseEntity *pAttacker );

	int iBlueFlagStatus;
	int iRedFlagStatus;

	int iBlueTeamScore;
	int iRedTeamScore; 

	float m_flFlagStatusTime;

private:
	void RecountTeams();

	BOOL m_DisableDeathMessages;
	BOOL m_DisableDeathPenalty;
	BOOL m_teamLimit;				// This means the server set only some teams as valid
	char m_szTeamList[TEAMPLAY_TEAMLISTLENGTH];
};
#endif // THREEWAVE_GAMERULES_H
