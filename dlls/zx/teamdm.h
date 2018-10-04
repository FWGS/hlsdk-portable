#ifndef TEAMDM_H
#define TEAMDM_H

#define TEAM1 "Red"
#define TEAM2 "Blue"

class CRulesTeamDM : public CHalfLifeMultiplay
{
public:
	CRulesTeamDM( );

	virtual void ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, BOOL bKill, BOOL bGib );
	virtual BOOL ClientCommand( CBasePlayer *pPlayer, const char *pcmd );
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor );
	virtual BOOL FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker );
	virtual const char *GetGameDescription( void ) { return "Team DM"; }	// this is the game name that gets seen in the server browser
	const char *GetIndexedTeamName( int teamIndex );
	virtual edict_t *GetPlayerSpawnSpot( CBasePlayer *pPlayer );
	const char *GetTeamID( CBaseEntity *pEntity );
	int GetTeamIndex( const char *pTeamName );
	virtual void InitHUD( CBasePlayer *pPlayer );
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );
	virtual int PlayersOnTeam( const char *szTeamName );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual BOOL ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target );
	virtual int TeamFrags( const char *szTeamName );
	virtual const char *TeamWithFewestPlayers( void );
	virtual void Think( void );
	virtual void UpdateGameMode( CBasePlayer *pPlayer );	// the client needs to be informed of the current game mode

private:
	BOOL m_DisableDeathMessages;
	BOOL m_DisableDeathPenalty;
};

#endif