
#ifndef STUKABAT_H
#define STUKABAT_H

#define		STUKABAT_IDLE				0
#define		STUKABAT_BORED				1
#define		STUKABAT_SCARED_BY_ENT		2
#define		STUKABAT_SMELL_FOOD			3
#define		STUKABAT_EAT				4
#define		STUKABAT_DEAD				5

//=========================================================
// Nodes at which Stukabats may land. 
//=========================================================
class CStukabatNode : public CBaseEntity
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );

	bool	mbInUse;

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
};

//=========================================================
// The Stukabat monster. 
//=========================================================
class CStukabat : public CFlyingMonster
{
public:
	enum SB_Medium
	{
		SB_ONGROUND,
		SB_INAIR,
		SB_ONCEILING
	};

	void  Spawn( void );
	void  Precache( void );
	void  SetYawSpeed( void );
	int   Classify( void );
	void  HandleAnimEvent( MonsterEvent_t *pEvent );
	CUSTOM_SCHEDULES;

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	virtual void SetActivity ( Activity NewActivity );

	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType ( int Type );

	void Killed( entvars_t *pevAttacker, int iGib );
	void BecomeDead( void );

	void EXPORT CombatUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	//void EXPORT SlashTouch( CBaseEntity *pOther );
	void Slash(void);

	void  StartTask( Task_t *pTask );
	void  RunTask( Task_t *pTask );

	BOOL  CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL  CheckRangeAttack1 ( float flDot, float flDist );
	BOOL  CheckRangeAttack2 ( float flDot, float flDist );

	float ChangeYaw( int speed );
	Activity GetStoppedActivity( void );

	virtual int	  CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist );// check validity of a straight move through space
	virtual void  Move( float flInterval );
	virtual void  MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval );
	void  MonsterThink( void );
	void  MonsterThinkInAir( void );
	void  MonsterThinkOnGround( void );
	void  MonsterThinkOnCeiling( void );
	void  Stop( void );
	void  Fly( void );
	Vector DoProbe(const Vector &Probe);

	void	LandGround (void);
	void	LandCeiling (void);
	void	TakeOffGround (void);
	void	FindNode (void);

	float VectorToPitch( const Vector &vec);
	float FlPitchDiff( void );
	float ChangePitch( int speed );

	Vector m_SaveVelocity;
	float m_idealDist;

	float m_flNextRangedAttack;
	float m_flNextMeleeAttack;
	//BOOL  m_bOnAttack;

	float m_flMaxSpeed;
	float m_flMinSpeed;
	float m_flMaxDist;

	float m_flNextAlert;
	float m_flLandTime;

	void PickNewDest ( int iCondition );

	SB_Medium	meMedium;
	int			m_iMode;

	CStukabatNode*	mpCeilingNode;

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pAttackSounds[];
	static const char *pSlashSounds[];
	static const char *pDieSounds[];
	static const char *pPainSounds[];

	void IdleSound( void );
	void AlertSound( void );
	void AttackSound( void );
	void SlashSound( void );
	void DeathSound( void );
	void PainSound( void );
};


#endif


