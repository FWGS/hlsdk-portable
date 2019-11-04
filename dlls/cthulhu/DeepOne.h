
#ifndef DEEP_ONE_H
#define DEEP_ONE_H

class CDeepOne : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	virtual float GetAttackDist( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int IgnoreConditions ( void );

	float m_flNextFlinch;

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	virtual int GetVoicePitch( void ) { return PITCH_NORM; };
	virtual float GetVolume (void)		{ return 0.5; };

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};

class CDagon : public CDeepOne
{
public:
	void Spawn( void );
	void Precache( void );
	virtual float GetAttackDist( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	virtual BOOL CheckMeleeAttack1( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	virtual int GetVoicePitch( void ) { return RANDOM_LONG(40,50); };
	virtual float GetVolume (void)		{ return 1.0; };
};

#endif
