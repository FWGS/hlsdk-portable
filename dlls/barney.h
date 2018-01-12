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
// monster template
//=========================================================
#ifndef BARNEY_H
#define BARNEY_H

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		BARNEY_AE_DRAW		( 2 )
#define		BARNEY_AE_SHOOT		( 3 )
#define		BARNEY_AE_HOLSTER	( 4 )
#define		BARNEY_AE_RELOAD	( 5 )
#define		BARNEY_AE_KICK		( 6 )

#define	BARNEY_BODY_GUNHOLSTERED	0
#define	BARNEY_BODY_GUNDRAWN		1
#define BARNEY_BODY_GUNGONE		2

#define SF_BARNEY_HAVESUIT		8
#define SF_BARNEY_LAZY_BARNEY		32
#define SF_BARNEY_CARDS_BARNEY		64

enum
{
	SCHED_BARNEY_COVER_AND_RELOAD = LAST_COMMON_SCHEDULE + 1
};

//=========================================================
// CBarney
//=========================================================
class CBarney : public CTalkMonster
{
public:
	virtual void Spawn( void );
	virtual void Precache( void );
	void SetYawSpeed( void );
	int ISoundMask( void );
	virtual void BarneyFirePistol( void );
	virtual void AlertSound( void );
	int Classify( void );
	virtual void HandleAnimEvent( MonsterEvent_t *pEvent );

	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	virtual int ObjectCaps( void ) { return CTalkMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	BOOL CheckRangeAttack1( float flDot, float flDist );

	virtual void DeclineFollowing( void );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType( int Type );
	virtual Schedule_t *GetSchedule( void );
	MONSTERSTATE GetIdealState( void );

	virtual void DeathSound( void );
	virtual void PainSound( void );

	virtual void TalkInit( void );
	void CheckAmmo( void );
	virtual void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	virtual void Killed( entvars_t *pevAttacker, int iGib );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	BOOL m_fGunDrawn;
	float m_painTime;
	float m_checkAttackTime;
	BOOL m_lastAttackCheck;

	// UNDONE: What is this for?  It isn't used?
	float m_flPlayerDamage;// how much pain has the player inflicted on me?

	CUSTOM_SCHEDULES
};
#endif
