//-------------------------------------------------
//-												---
//-			lflammes.h							---
//-												---
//-------------------------------------------------
//			par Julien		-----------------------
//-------------------------------------------------
//- header du lance flammes	-----------------------
//-------------------------------------------------


#ifndef LFLAMMES_H
#define LFLAMMES_H


//----------------------------------------
// classe des boules de feu


#define FLAMME_LIBRE			0
#define	FLAMME_ATTACHEE			1
#define	DETRUIT_FLAMME			2
#define	FLAMME_DECO				3
#define	FLAMME_DEAD				4

#define FLAMME_RADIUS_SMALL		10
#define FLAMME_RADIUS_BIG		32

#define FLAMME_DAMAGE_MONSTER	0.5
#define FLAMME_DAMAGE_PLAYER	40
#define PLAYER_BURN_TIME		5






class CFlamme : public CPointEntity
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );

	BOOL CanCatchMonster ( CBaseMonster *pMonster );
	float FlameDamageMonster ( CBaseMonster *pMonster );

	static CFlamme *CreateFlamme( Vector vecOrigin, Vector vecAngles, int imode = FLAMME_LIBRE );

	void EXPORT FlameThink ( void );
	void EXPORT FlameTouch( CBaseEntity *pOther );

	float	m_flBirthTime;
	int		m_iMode;
	float	m_flPlayerDmg;
	int		m_bRestore;
	float	m_flMonsterDamage;
};


#endif // LFLAMMES_H
