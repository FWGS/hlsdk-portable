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

#ifndef SCIENTIST_H
#define SCIENTIST_H

#if defined ( THEGATE_DLL )
#define		NUM_SCIENTIST_HEADS		4 // four heads available for scientist model
enum { HEAD_GLASSES = 0, HEAD_EINSTEIN = 1, HEAD_LUTHER = 2, HEAD_SLICK = 3 };

//=======================================================
// Scientist
//=======================================================

class CScientist : public CTalkMonster
{
public:
	void Spawn( void );
	void Precache( void );

	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	int	ObjectCaps( void ) { return CTalkMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	virtual int FriendNumber( int arrayNumber );
	void SetActivity ( Activity newActivity );
	Activity GetStoppedActivity( void );
	int ISoundMask( void );
	void DeclineFollowing( void );

	float	CoverRadius( void ) { return 1200; }		// Need more room for cover because scientists want to get far away!
	BOOL	DisregardEnemy( CBaseEntity *pEnemy ) { return !pEnemy->IsAlive() || (gpGlobals->time - m_fearTime) > 15; }

	BOOL	CanHeal( void );
	void	Heal( void );
	void	Scream( void );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	void DeathSound( void );
	void PainSound( void );
	
	void TalkInit( void );

	void			Killed( entvars_t *pevAttacker, int iGib );
	
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;

private:	
	float m_painTime;
	float m_healTime;
	float m_fearTime;
};


//=========================================================
// Dead Scientist PROP
//=========================================================
class CDeadScientist : public CBaseMonster
{
public:
	void Spawn(void);
	int	Classify(void) { return	CLASS_HUMAN_PASSIVE; }

	void KeyValue(KeyValueData *pkvd);
	int	m_iPose;// which sequence to display
	static char *m_szPoses[7];
};

//=========================================================
// Sitting Scientist PROP
//=========================================================
class CSittingScientist : public CScientist // kdb: changed from public CBaseMonster so he can speak
{
public:
	void Spawn( void );
	void  Precache( void );

	void EXPORT SittingThink( void );
	int	Classify ( void );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	virtual void SetAnswerQuestion( CTalkMonster *pSpeaker );
	int FriendNumber( int arrayNumber );

	int FIdleSpeak ( void );
	int		m_baseSequence;	
	int		m_headTurn;
	float	m_flResponseDelay;
};


#endif // defined ( THEGATE_DLL )

#endif // SCIENTIST_H