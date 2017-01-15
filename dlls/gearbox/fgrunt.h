/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef FGRUNT_H
#define FGRUNT_H


//=========================================================
// CFGrunt
//=========================================================
class CFGrunt : public CTalkMonster
{
public:
#if 1
	virtual void KeyValue(KeyValueData *pkvd);
#endif

	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  ISoundMask(void);
	void AlertSound(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	void PrescheduleThink( void );

	void RunTask(Task_t *pTask);
	void StartTask(Task_t *pTask);
	virtual int	ObjectCaps(void) { return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	BOOL FCanCheckAttacks(void);
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist);

	void DeclineFollowing(void);

	// AI functions
	void SetActivity(Activity newActivity);

	// Override these to set behavior
	Schedule_t *GetScheduleOfType(int Type);
	Schedule_t *GetSchedule(void);
	Schedule_t *GetSquadSchedule(void);
	MONSTERSTATE GetIdealState(void);

	void DeathSound(void);
	void PainSound(void);

	virtual void TalkInit(void);

	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed(entvars_t *pevAttacker, int iGib);

#if 1

	virtual BOOL IsMedic(void) const { return FALSE; }

	virtual CBaseEntity* DropGun(const Vector& vecSrc, const Vector& vecAngles, char* szClassname = NULL);

	void Fire(
		const Vector& vecShootOrigin,
		const Vector& vecShoorDir,
		const Vector& vecSpread,
		int model,
		int effects = EF_MUZZLEFLASH,
		int bulletType = BULLET_MONSTER_MP5,
		int soundType = TE_BOUNCE_SHELL);

	Vector GetGunPosition(void);
	void Shoot(void);
	void Shotgun(void);
	void Saw(void);

	virtual void GibMonster(void);
	void SpeakSentence(void);

	int IRelationship(CBaseEntity *pTarget);

	virtual BOOL FOkToSpeak(void);
	void JustSpoke(void);

	CBaseEntity	*Kick(void);
#endif

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	float	m_painTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;

	// UNDONE: What is this for?  It isn't used?
	float	m_flPlayerDamage;// how much pain has the player inflicted on me?



	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	Vector	m_vecTossVelocity;

	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.
	int		m_cClipSize;

	int m_voicePitch;

	int		m_iBrassShell;
	int		m_iShotgunShell;
	int		m_iSawShell;

	int		m_iSentence;

	static const char *pGruntSentences[];

	CUSTOM_SCHEDULES;

	int head;
	int torso;
};


#endif // FGRUNT_H