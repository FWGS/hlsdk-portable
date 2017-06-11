//
// heavyrain_gamerules.h
//

#define JASON 0;

class CHeavyRainplay : public CHalfLifeMultiplay
{
public:
	CHeavyRainplay();
	virtual const char *GetGameDescription( void ) { return "Heavy Rain Mode"; }  // this is the game name that gets seen in the server browser
	virtual BOOL IsHeavyRain( void );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual int JasonsStolen( int jason );
};