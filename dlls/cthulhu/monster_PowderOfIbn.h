
#ifndef MONSTER_POWDER_IBN_H
#define MONSTER_POWDER_IBN_H


// Dynamite
class CMonsterPowderOfIbn : public CBaseMonster
{
public:
	void Spawn( void );

	static CMonsterPowderOfIbn *ShootContact( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );

	void Explode( Vector vecSrc, Vector vecAim );
	void Explode( TraceResult *pTrace, int bitsDamageType );
	void EXPORT Smoke( void );

	void EXPORT BounceTouch( CBaseEntity *pOther );
	void EXPORT ExplodeTouch( CBaseEntity *pOther );
	void EXPORT DangerSoundThink( void );
	void EXPORT PreDetonate( void );
	void EXPORT Detonate( void );
	void EXPORT TumbleThink( void );

	virtual void BounceSound( void );
	virtual int	BloodColor( void ) { return DONT_BLEED; }
	virtual void Killed( entvars_t *pevAttacker, int iGib );

	BOOL m_fRegisteredSound;// whether or not this grenade has issued its DANGER sound to the world sound list yet.
};

#endif
