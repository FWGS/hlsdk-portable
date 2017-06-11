// Shitty co-op addition. Bypasses a few things.
// Buggy as hell, and YOU should fix it. I dont have the time.

#define JASON 0;

class CCoopplay : public CHalfLifeMultiplay
{
public:
	CCoopplay();
	virtual const char *GetGameDescription( void ) { return "Co-Op"; }  // this is the game name that gets seen in the server browser
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual BOOL IsDeathmatch( void );
	virtual BOOL IsCoOp( void );
};