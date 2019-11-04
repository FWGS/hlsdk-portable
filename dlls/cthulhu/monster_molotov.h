
#ifndef MONSTER_MOLOTOV_H
#define MONSTER_MOLOTOV_H

#include "effects.h"
#include "triggers.h"
#include "bmodels.h"

const int MAX_MONSTER = 1000;

// Molotov burning area
class CMonsterMolotov : public CBaseMonster
{
public:
	void Spawn( void );

	static CMonsterMolotov *ShootContact( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );

	void Explode( Vector vecSrc, Vector vecAim );
	void Explode( TraceResult *pTrace, int bitsDamageType );
	void MakeEffects( void );
	void EXPORT BurnOut( void );
	void EXPORT Burning( void );

	void EXPORT ExplodeTouch( CBaseEntity *pOther );
	void EXPORT DangerSoundThink( void );
	void EXPORT PreDetonate( void );
	void EXPORT Detonate( void );
	void EXPORT DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT TumbleThink( void );

	virtual int	BloodColor( void ) { return DONT_BLEED; }
	virtual void Killed( entvars_t *pevAttacker, int iGib );
	void Precache( void );

	int	Save( CSave &save ); 
	int	Restore( CRestore &restore );
	//static TYPEDESCRIPTION m_SaveData[];

	CSprite*			m_pFireSprite[9];
	CTriggerHurt*		m_pFireHurt;
	//CFuncBurningClip*	m_pBurningArea;
	BOOL Repel ( CBaseMonster* pEnt );

protected:
	CBaseEntity* pEntInSphere[MAX_MONSTER];
};

#endif
