
#ifndef CHICKEN_H
#define CHICKEN_H


class CChickenFeathers : public CBaseMonster
{
public:
	void Spawn ( void );
	void Die( void );
	int	 Classify ( void );
	virtual int	ObjectCaps( void ) { return (CBaseMonster :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }
};




class CChicken : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	void EXPORT MonsterThink ( void );
	void Move ( float flInterval );
	void PickNewDest ( int iCondition );
	void Killed( entvars_t *pevAttacker, int iGib );

	float	m_flNextSmellTime;
	int		Classify ( void );
	int		ISoundMask ( void );

	virtual int HasCustomGibs( void ) { return m_iszGibModel; }
	
	int m_iszGibModel;

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	// UNDONE: These don't necessarily need to be save/restored, but if we add more data, it may
	int		m_iMode;
	// -----------------------------
};


#endif

