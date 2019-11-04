
#ifndef TREX_H
#define TREX_H

class CTRexStomp : public CBaseEntity
{
public:
	void Spawn( void );
	void Think( void );
	static CTRexStomp *StompCreate( const Vector &origin, const Vector &end, float speed );

private:
// UNDONE: re-use this sprite list instead of creating new ones all the time
//	CSprite		*m_pSprites[ STOMP_SPRITE_COUNT ];
};

class CTRex : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void Killed( entvars_t *pevAttacker, int iGib );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	BOOL CheckMeleeAttack1( float flDot, float flDist );		// Swipe
	BOOL CheckMeleeAttack2( float flDot, float flDist );		// Flames
	BOOL CheckRangeAttack1( float flDot, float flDist );		// Stomp attack
	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector( -80, -80, 0 );
		pev->absmax = pev->origin + Vector( 80, 80, 214 );
	}

	Schedule_t *GetScheduleOfType( int Type );
	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );

	void Leap( void );
	void StompAttack( void );
	void FlameCreate( void );
	void FlameUpdate( void );
	void FlameControls( float angleX, float angleY );
	void FlameDestroy( void );
	inline BOOL FlameIsOn( void ) { return m_pFlame[0] != NULL; }

	void FlameDamage( Vector vecStart, Vector vecEnd, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;

private:
	static const char *pAttackHitSounds[];
	static const char *pBeamAttackSounds[];
	static const char *pAttackMissSounds[];
	static const char *pRicSounds[];
	static const char *pFootSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackSounds[];
	static const char *pStompSounds[];
	static const char *pBreatheSounds[];
	static const char *pRoarSounds[];

	CBaseEntity* TRexCheckTraceHullAttack(float flDist, int iDamage, int iDmgType);

	CBeam		*m_pFlame[2];		// Flame beams

	float		m_seeTime;			// Time to attack (when I see the enemy, I set this)
	float		m_flameTime;		// Time of next flame attack
	float		m_painSoundTime;	// Time of next pain sound
	float		m_streakTime;		// streak timer (don't send too many)
	float		m_flameX;			// Flame thrower aim
	float		m_flameY;			
};


#endif

