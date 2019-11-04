
#ifndef PRIEST_H
#define PRIEST_H

class CPriest : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int	 ISoundMask( void );
	int  Classify ( void );
	int  IRelationship( CBaseEntity *pTarget );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	void CallForHelp( char *szClassname, float flDist, EHANDLE hEnemy, Vector &vecLocation );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	Vector GetGunPosition( void );

	void DeathSound( void );
	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );

	void Killed( entvars_t *pevAttacker, int iGib );
	void GibMonster ( void );

	void SetActivity ( Activity NewActivity );
    void StartTask ( Task_t *pTask );
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType ( int Type );
	CUSTOM_SCHEDULES;

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void ClearBeams( );
	void ArmBeam( int side );
	void WackBeam( int side, CBaseEntity *pEntity );
	void BeamGlow( void );
	void ZapBeam( int side );

	CBeam *m_pBeam[PRIEST_MAX_BEAMS];

	int m_iBeams;
	float m_flNextAttack;

	BOOL	m_fStanding;
	int	m_voicePitch;

	BOOL FOkToSpeak( void );
	void JustSpoke( void );
	void SpeakSentence( void );

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];

	int		m_iSentence;
	static const char *pPriestSentences[];
};




#endif

