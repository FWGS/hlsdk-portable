#ifndef SHOCKBEAM_H
#define SHOCKBEAM_H

//=========================================================
// Shockrifle projectile
//=========================================================
class CShock : public CBaseAnimating
{
public:
	void Spawn(void);
	void Precache();

	static void Shoot(entvars_t *pevOwner, const Vector angles, const Vector vecStart, const Vector vecVelocity);
	void Touch(CBaseEntity *pOther);
	void EXPORT FlyThink();

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void CreateEffects();
	void ClearEffects();
	void UpdateOnRemove();

	CBeam *m_pBeam;
	CBeam *m_pNoise;
	CSprite *m_pSprite;
};
#endif
