//=========================================================
// Opposing Forces Monster Shocktrooper\Shockrifle blast
//
// Made by Demiurge
//
//=========================================================

//=========================================================
// Shock Defines
#define SHOCK_AIR_VELOCITY		20000

class CShock : public CBaseMonster
{
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	void EXPORT ShockTouch( CBaseEntity *pOther );
	void EXPORT ExplodeThink( void );
	void EXPORT BlastOn( void );
	void EXPORT BlastOff( void );
	void UpdateOnRemove();

	CBeam *m_pBeam;
	CBeam *m_pNoise;
	CSprite *m_pSprite;
	int m_iTrail;
	Vector m_vecForward;

public:
	static CShock *ShockCreate( void );
};

