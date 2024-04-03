class CTriggerKicker : public CBaseDelay
{
public:
	void Spawn( void );
	void Think( void );
	void KickPlayer( CBasePlayer *pKickMe );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
private:
	CBasePlayer *pPlayerToKick;
};

class CTriggerLockedMission : public CBaseDelay
{
public:
	void Spawn( void );
	void Think( void );
	void Lock( void );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};