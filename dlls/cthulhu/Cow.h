
#ifndef COW_H
#define COW_H

class CCow : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void Killed( entvars_t *pevAttacker, int iGib );
	void IdleSound( void );

	int		Classify ( void );
	
	virtual int HasCustomGibs( void ) { return m_iszGibModel; }
	
	int m_iszGibModel;

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
};


#endif

