
#ifndef TRIPMINE_H
#define TRIPMINE_H

class CTripmineGrenade : public CGrenade
{
	void Spawn( void );
	void Precache( void );
	void UpdateOnRemove();

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	
	void EXPORT WarningThink( void );
	void EXPORT PowerupThink( void );
	void EXPORT BeamBreakThink( void );
	void EXPORT DelayDeathThink( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	void MakeBeam( void );
	void KillBeam( void );

	float		m_flPowerUp;
	Vector		m_vecDir;
	Vector		m_vecEnd;
	float		m_flBeamLength;

	EHANDLE		m_hOwner;
	CBeam		*m_pBeam;
	Vector		m_posOwner;
	Vector		m_angleOwner;
	edict_t		*m_pRealOwner;// tracelines don't hit PEV->OWNER, which means a player couldn't detonate his own trip mine, so we store the owner here.

	// BMOD Begin - New tripmine functions
	BOOL BMOD_IsSpawnMine( void );
	void FlashBang( void );
public:
	void Deactivate( void );
	edict_t *Owner( void ) { return m_pRealOwner; };
	BOOL m_bIsFlashbang;	
	// BMOD End - New tripmine functions

};

#endif
