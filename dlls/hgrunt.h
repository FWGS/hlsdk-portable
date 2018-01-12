/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// hgrunt
//=========================================================

typedef enum
{
        HGRUNT_SENT_NONE = -1,
        HGRUNT_SENT_GREN = 0,
        HGRUNT_SENT_ALERT,
        HGRUNT_SENT_MONSTER,
        HGRUNT_SENT_COVER,
        HGRUNT_SENT_THROW,
        HGRUNT_SENT_CHARGE,
        HGRUNT_SENT_TAUNT
} HGRUNT_SENTENCE_TYPES;

class CHGrunt : public CSquadMonster
{
public:
	virtual void Spawn( void );
	virtual void Precache( void );
	void SetYawSpeed( void );
	int Classify( void );
	int ISoundMask( void );
	virtual void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks( void );
	BOOL CheckMeleeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack2( float flDot, float flDist );
	void CheckAmmo( void );
	void SetActivity( Activity NewActivity );
	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );
	void DeathSound( void );
	void PainSound( void );
	virtual void IdleSound( void );
	Vector GetGunPosition( void );
	virtual void Shoot( void );
	virtual void Shotgun( void );
	void PrescheduleThink( void );
	virtual void GibMonster( void );
	virtual void SpeakSentence( void );

	int Save( CSave &save ); 
	int Restore( CRestore &restore );

	CBaseEntity *Kick( void );
	virtual Schedule_t *GetSchedule( void );
	virtual Schedule_t *GetScheduleOfType( int Type );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	virtual int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	virtual int IRelationship( CBaseEntity *pTarget );

	BOOL FOkToSpeak( void );
	void JustSpoke( void );
	BOOL IsNotProvoked();
	void Provoke();

	CUSTOM_SCHEDULES
	static TYPEDESCRIPTION m_SaveData[];

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	Vector m_vecTossVelocity;

	BOOL m_fThrowGrenade;
	BOOL m_fStanding;
	BOOL m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.
	int m_cClipSize;

	int m_voicePitch;

	int m_iBrassShell;
	int m_iShotgunShell;

	int m_iSentence;

	static const char *pGruntSentences[];
};
