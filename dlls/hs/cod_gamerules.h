class CCodplay : public CHalfLifeMultiplay
{
public:
	CCodplay();
	virtual const char *GetGameDescription( void ) { return "Cawadooty"; }  // this is the game name that gets seen in the server browser
	virtual BOOL IsCOD( void );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
};