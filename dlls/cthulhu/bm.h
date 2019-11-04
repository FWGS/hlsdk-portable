
#ifndef BM_H
#define BM_H

//LRC brought in from animation.h
#define ACTIVITY_NOT_AVAILABLE		-1

#define SF_INFOBM_RUN		0x0001
#define SF_INFOBM_WAIT		0x0002

// AI Nodes for Big Momma
class CInfoBM : public CPointEntity
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData* pkvd );

	// name in pev->targetname
	// next in pev->target
	// radius in pev->scale
	// health in pev->health
	// Reach target in pev->message
	// Reach delay in pev->speed
	// Reach sequence in pev->netname
	
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int		m_preSequence;
};


class CBMortar : public CBaseEntity
{
public:
	void Spawn( void );

	static CBMortar *Shoot( edict_t *pOwner, Vector vecStart, Vector vecVelocity );
	void Touch( CBaseEntity *pOther );
	void EXPORT Animate( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int  m_maxFrame;
};

Vector VecCheckSplatToss( entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float maxHeight );
void MortarSpray( const Vector &position, const Vector &direction, int spriteModel, int count );



#endif
