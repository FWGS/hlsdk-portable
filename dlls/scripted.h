/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
#ifndef SCRIPTED_H
#define SCRIPTED_H

#ifndef SCRIPTEVENT_H
#include "scriptevent.h"
#endif

#define SF_SCRIPT_WAITTILLSEEN		1
#define SF_SCRIPT_EXITAGITATED		2
#define SF_SCRIPT_REPEATABLE		4
#define SF_SCRIPT_LEAVECORPSE		8
//#define SF_SCRIPT_INTERPOLATE		16 // don't use, old bug
#define SF_SCRIPT_NOINTERRUPT		32
#define SF_SCRIPT_OVERRIDESTATE		64
#define SF_SCRIPT_NOSCRIPTMOVEMENT	128
#define SF_SCRIPT_STAYDEAD			256 // LRC- signifies that the animation kills the monster
										// (needed because the monster animations don't use AnimEvent 1000 properly)

#define SCRIPT_BREAK_CONDITIONS		(bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE)

//LRC - rearranged into flags
#define SS_INTERRUPT_IDLE		0x0
#define SS_INTERRUPT_ALERT		0x1
#define SS_INTERRUPT_ANYSTATE	0x2
#define SS_INTERRUPT_SCRIPTS	0x4

// when a monster finishes an AI scripted sequence, we can choose
// a schedule to place them in. These defines are the aliases to
// resolve worldcraft input to real schedules (sjb)
#define SCRIPT_FINISHSCHED_DEFAULT	0
#define SCRIPT_FINISHSCHED_AMBUSH	1

class CCineMonster : public CBaseMonster
{
public:
	void Spawn( void );
	virtual void KeyValue( KeyValueData *pkvd );
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void Blocked( CBaseEntity *pOther );
	virtual void Touch( CBaseEntity *pOther );
	virtual int	 ObjectCaps( void ) { return (CBaseMonster :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }
	virtual void Activate( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

	//LRC: states for script entities
	virtual STATE	GetState( void ) { return m_iState; };
	STATE	m_iState;

	// void EXPORT CineSpawnThink( void );
	void EXPORT CineThink( void );
	void EXPORT InitIdleThink( void ); //LRC
	void Pain( void );
	void Die( void );
	void DelayStart( int state );
	CBaseMonster* FindEntity( const char* sName, CBaseEntity *pActivator );
	virtual void PossessEntity( void );

	inline BOOL IsAction( void ) { return FClassnameIs(pev,"scripted_action"); }; //LRC

	//LRC: Should the monster do a precise attack for this scripted_action?
	// (Do a precise attack if we'll be turning to face the target, but we haven't just walked to the target.)
	BOOL PreciseAttack( void )
	{
	//	if (m_fTurnType != 1) { ALERT(at_console,"preciseattack fails check 1\n"); return FALSE; }
	//	if (m_fMoveTo == 0) { ALERT(at_console,"preciseattack fails check 2\n"); return FALSE; }
	//	if (m_fMoveTo != 5 && m_iszAttack == 0) { ALERT(at_console,"preciseattack fails check 3\n"); return FALSE; }
	//	ALERT(at_console,"preciseattack passes!\n");
	//	return TRUE;
		return m_fTurnType == 1 && ( m_fMoveTo == 5 || (m_fMoveTo != 0 && !FStrEq(STRING(m_iszAttack), STRING(m_iszMoveTarget)) ));
	};

	void ReleaseEntity( CBaseMonster *pEntity );
	void CancelScript( void );
	virtual BOOL StartSequence( CBaseMonster *pTarget, int iszSeq, BOOL completeOnEmpty );
	void SequenceDone ( CBaseMonster *pMonster );
	virtual void FixScriptMonsterSchedule( CBaseMonster *pMonster );
	BOOL	CanInterrupt( void );
	void	AllowInterrupt( BOOL fAllow );
	int		IgnoreConditions( void );

	int	m_iszIdle;		// string index for idle animation
	int	m_iszPlay;		// string index for scripted animation
	int m_iszEntity;	// entity that is wanted for this script
	int m_iszAttack;	// entity to attack
	int m_iszMoveTarget; // entity to move to
	int m_iszFireOnBegin; // entity to fire when the sequence _starts_.
	int m_fMoveTo;
	int m_fTurnType;
	int m_fAction;
	int m_iFinishSchedule;
	float m_flRadius;		// range to search
//LRC- this does nothing!!	float m_flRepeat;	// repeat rate
	int m_iRepeats; //LRC - number of times to repeat the animation
	int m_iRepeatsLeft; //LRC
	float m_fRepeatFrame; //LRC
	int m_iPriority; //LRC

	int m_iDelay;
	float m_startTime;

	int	m_saved_movetype;
	int	m_saved_solid;
	int m_saved_effects;
//	Vector m_vecOrigOrigin;
	BOOL m_interruptable;
};

//LRC - removed CCineAI, obsolete

#endif		//SCRIPTED_H
