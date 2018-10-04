//=========================================================
// Opposing Forces Monster Voltigore blast
//
// Made by Demiurge
//
//=========================================================

//=========================================================
// Shockball Defines
//
class CShockball : public CBaseMonster
{
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	void EXPORT ShockballTouch( CBaseEntity *pOther );
	void EXPORT ExplodeThink( void );
	void EXPORT FlyThink( void );
	void EXPORT BlastOn( void );
	void EXPORT BlastOff( void );

	CBeam *m_pBeam;
	CBeam *m_pNoise;
	CSprite *m_pSprite;
	int m_iTrail;
	int m_iBlastText;
	Vector m_vecForward;
	Vector m_vecUp;

public:
	static CShockball *ShockballCreate( void );
};
