
#ifndef ALIEN_EGG
#define ALIEN_EGG


class CAlienEgg : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void RunTask ( Task_t *pTask );
	void StartTask ( Task_t *pTask );
	void SetYawSpeed ( void );
	Vector Center( void );
	Vector BodyTarget( const Vector &posSrc );
	int  Classify ( void );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	virtual void Killed( entvars_t *pevAttacker, int iGib );
	void Burst();
	void EXPORT EggUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual Schedule_t *GetSchedule( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

	int m_iOrientation;		// 0 = floor, 1 = ceiling
};


#endif
