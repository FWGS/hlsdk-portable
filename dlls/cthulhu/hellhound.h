
#ifndef HELLHOUND_H
#define HELLHOUND_H

//=========================================================
// Hellhound's fireball
//=========================================================
class CHHFireball : public CBaseEntity
{
public:
	void Spawn( void );

	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	void Touch( CBaseEntity *pOther );
	void EXPORT Animate( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int  m_maxFrame;
};

//=========================================================
// Hellhound
//=========================================================
class CHellhound : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int IgnoreConditions ( void );
	virtual int  ISoundMask( void );

	float m_flNextFlinch;

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

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

	// No range attacks
	virtual BOOL CheckRangeAttack1 ( float flDot, float flDist );
	virtual BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; };

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	float m_flNextFireballTime;// next time the hellhound may use the fireball attack.
};



#endif


