
#ifndef STUKAGRENADE_H
#define STUKAGRENADE_H

#include "effects.h"

// Contact Grenade
class CStukaGrenade : public CBaseMonster
{
public:
	void Spawn( void );

	static CStukaGrenade *ShootContact( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );

	void Explode( Vector vecSrc, Vector vecAim );
	void Explode( TraceResult *pTrace, int bitsDamageType );
	void EXPORT Smoke( void );

	void EXPORT ExplodeTouch( CBaseEntity *pOther );
	void EXPORT PreDetonate( void );
	void EXPORT Detonate( void );
	void EXPORT TumbleThink( void );
	void EXPORT DangerSoundThink( void );

	virtual int	BloodColor( void ) { return DONT_BLEED; }
	virtual void Killed( entvars_t *pevAttacker, int iGib );
};

#endif
