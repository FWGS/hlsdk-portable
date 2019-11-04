
#ifndef SNAKE_H
#define SNAKE_H

class CSnake : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int IgnoreConditions ( void );
	int  IRelationship( CBaseEntity *pTarget );
	void SetOwner( CBaseEntity* pOwner );

	float m_flNextFlinch;

	void AttackSound( void );
	void IdleSound( void );
	void PainSound( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pPainSounds[];

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	virtual int HasCustomGibs( void ) { return m_iszGibModel; }
	
	int m_iszGibModel;

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

protected:

	float	m_fNextBiteTime;
	CBaseEntity*	mpOwner;
};


#endif
