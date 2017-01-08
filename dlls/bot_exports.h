void InstallBotControl();
void Bot_ServerCommand();
void Bot_RegisterCvars();

class CBotManager
{
public:
	CBotManager();
	virtual ~CBotManager(){}

	virtual void ClientDisconnect(CBasePlayer *pPlayer) = 0;
	virtual BOOL ClientCommand(CBasePlayer *pPlayer, const char *pcmd) = 0;

	virtual void ServerActivate() = 0;
	virtual void ServerDeactivate() = 0;

	virtual void ServerCommand(const char *pcmd) = 0;
	virtual void AddServerCommand(const char *cmd) = 0;
	virtual void AddServerCommands() = 0;

	virtual void RestartRound();
	virtual void StartFrame();

	// Events are propogated to all bots.
//	virtual void OnEvent(GameEventType event, CBaseEntity *entity = NULL, CBaseEntity *other = NULL);		// Invoked when event occurs in the game (some events have NULL entity).
	virtual unsigned int GetPlayerPriority(CBasePlayer *player) const = 0;						// return priority of player (0 = max pri)

public:
	const char *GetNavMapFilename() const;										// return the filename for this map's "nav" file

//	void AddGrenade(int type, CGrenade *grenade);									// add an active grenade to the bot's awareness
	//void RemoveGrenade(CGrenade *grenade);										// the grenade entity in the world is going away
	void ValidateActiveGrenades();										// destroy any invalid active grenades
	void DestroyAllGrenades();

	bool IsLineBlockedBySmoke(const Vector *from, const Vector *to);						// return true if line intersects smoke volume
	bool IsInsideSmokeCloud(const Vector *pos);									// return true if position is inside a smoke cloud

private:
	// the list of active grenades the bots are aware of
//	ActiveGrenadeList m_activeGrenadeList;
};

extern CBotManager *TheBots;