
class CEihortVictim : public CTalkMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  ISoundMask( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int  Classify ( void );
	
	void StartTask( Task_t *pTask );

	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	void DeathSound( void );
	void PainSound( void );
	
	void TalkInit( void );

	void Killed( entvars_t *pevAttacker, int iGib );

	void Burst ( void );
	
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

//	BOOL	m_fGunDrawn;
	float	m_painTime;
};

