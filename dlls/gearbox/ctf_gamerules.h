

#ifndef CTF_GAMERULES_H
#define CTF_GAMERULES_H

#define BLUE 2
#define RED 1
#ifndef NO_VOICEGAMEMGR
#include "voice_gamemgr.h"
#endif

// Standard Scoring
#define TEAM_CAPTURE_CAPTURE_BONUS 5 // what you get for capture
#define TEAM_CAPTURE_TEAM_BONUS  10 // what your team gets for capture
#define TEAM_CAPTURE_RECOVERY_BONUS  1 // what you get for recovery
#define TEAM_CAPTURE_FLAG_BONUS  0 // what you get for picking up enemy flag
#define TEAM_CAPTURE_FRAG_CARRIER_BONUS  2 // what you get for fragging a enemy flag carrier
#define TEAM_CAPTURE_FLAG_RETURN_TIME 40 // seconds until auto return

// bonuses
#define TEAM_CAPTURE_CARRIER_DANGER_PROTECT_BONUS  2 // bonus for fraggin someone
// who has recently hurt your flag carrier
#define TEAM_CAPTURE_CARRIER_PROTECT_BONUS 1 // bonus for fraggin someone while
// either you or your target are near your flag carrier
#define TEAM_CAPTURE_FLAG_DEFENSE_BONUS 1 // bonus for fraggin someone while
// either you or your target are near your flag
#define TEAM_CAPTURE_RETURN_FLAG_ASSIST_BONUS 1 // awarded for returning a flag that causes a
// capture to happen almost immediately
#define TEAM_CAPTURE_FRAG_CARRIER_ASSIST_BONUS 2 // award for fragging a flag carrier if a
// capture happens almost immediately

// Radius
#define TEAM_CAPTURE_TARGET_PROTECT_RADIUS 550 // the radius around an object being
// defended where a target will be worth extra frags
#define TEAM_CAPTURE_ATTACKER_PROTECT_RADIUS 550 // the radius around an object being
// defended where an attacker will get extra frags when making kills

// timeouts
#define TEAM_CAPTURE_CARRIER_DANGER_PROTECT_TIMEOUT 4
#define TEAM_CAPTURE_CARRIER_FLAG_SINCE_TIMEOUT 2
#define TEAM_CAPTURE_FRAG_CARRIER_ASSIST_TIMEOUT 6
#define TEAM_CAPTURE_RETURN_FLAG_ASSIST_TIMEOUT 4


// Steal sounds
#define STEAL_SOUND 1
#define CAPTURE_SOUND 2
#define RETURN_SOUND 3

#define RED_FLAG_STOLEN 1
#define BLUE_FLAG_STOLEN 2
#define RED_FLAG_CAPTURED 3
#define BLUE_FLAG_CAPTURED 4
#define RED_FLAG_RETURNED_PLAYER 5
#define BLUE_FLAG_RETURNED_PLAYER 6
#define RED_FLAG_RETURNED 7
#define BLUE_FLAG_RETURNED 8
#define RED_FLAG_LOST 9
#define BLUE_FLAG_LOST 10

#define RED_FLAG_STOLEN			1
#define BLUE_FLAG_STOLEN		2
#define RED_FLAG_DROPPED		3
#define BLUE_FLAG_DROPPED		4
#define RED_FLAG_ATBASE			5
#define BLUE_FLAG_ATBASE		6


#define MAX_TEAMNAME_LENGTH	16
#define MAX_TEAMS			32

#define TEAMPLAY_TEAMLISTLENGTH		MAX_TEAMS*MAX_TEAMNAME_LENGTH

#define PLAYER_MAX_HEALTH_VALUE	100
#define PLAYER_MAX_ARMOR_VALUE	100

class CCTFMultiplay : public CHalfLifeMultiplay
{
public:
	CCTFMultiplay();

	virtual BOOL ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128]);
	virtual BOOL ClientCommand(CBasePlayer *pPlayer, const char *pcmd);
	virtual void ClientUserInfoChanged(CBasePlayer *pPlayer, char *infobuffer);
	virtual BOOL IsTeamplay(void);
	virtual BOOL FPlayerCanTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker);
	virtual int PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget);
	virtual const char *GetTeamID(CBaseEntity *pEntity);
	virtual BOOL ShouldAutoAim(CBasePlayer *pPlayer, edict_t *target);
	virtual int IPointsForKill(CBasePlayer *pAttacker, CBasePlayer *pKilled);
	virtual void InitHUD(CBasePlayer *pl);
	virtual void DeathNotice(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor);
	virtual const char *GetGameDescription(void) { return "3Wave CTF"; }  // this is the game name that gets seen in the server browser
	virtual void UpdateGameMode(CBasePlayer *pPlayer);  // the client needs to be informed of the current game mode
	virtual void PlayerKilled(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor);
	virtual void Think(void);
	virtual int GetTeamIndex(const char *pTeamName);
	virtual const char *GetIndexedTeamName(int teamIndex);
	virtual BOOL IsValidTeam(const char *pTeamName);
	virtual void ChangePlayerTeam(CBasePlayer *pPlayer, int iTeam);
	virtual void PlayerSpawn(CBasePlayer *pPlayer);
	void JoinTeam(CBasePlayer *pPlayer, int iTeam);
	int TeamWithFewestPlayers(void);
	virtual void ClientDisconnected(edict_t *pClient);
	void GetFlagStatus(CBasePlayer *pPlayer);

	virtual edict_t *GetPlayerSpawnSpot(CBasePlayer *pPlayer);

	virtual void PlayerThink(CBasePlayer *pPlayer);

	void PlayerTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker);

	int iBlueFlagStatus;
	int iRedFlagStatus;

	int iBlueTeamScore;
	int iRedTeamScore;

	float m_flFlagStatusTime;
#ifndef NO_VOICEGAMEMGR
	CVoiceGameMgr	m_VoiceGameMgr;
#endif
private:
	void RecountTeams(void);

	BOOL m_DisableDeathMessages;
	BOOL m_DisableDeathPenalty;
	BOOL m_teamLimit;				// This means the server set only some teams as valid
	char m_szTeamList[TEAMPLAY_TEAMLISTLENGTH];
};
#endif // CTF_GAMERULES_H
