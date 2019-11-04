
#ifndef SHRIVELLING_H
#define SHRIVELLING_H

class CShrivelling : public CBasePlayerWeapon
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );

	void CreateEffect( void );
	void UpdateEffect( const Vector &startPoint, const Vector &endPoint, float timeBlend );
	void DestroyEffect( void );

	void EndAttack( void );
	void Attack( void );
	void PrimaryAttack( void );
	void WeaponIdle( void );
	static int g_fireAnims1[];
	static int g_fireAnims2[];

	float m_flAmmoUseTime;// since we use < 1 point of ammo per update, we subtract ammo on a timer.

	float GetPulseInterval( void );
	float GetDischargeInterval( void );

	void Fire( const Vector &vecOrigSrc, const Vector &vecDir );

	enum SHRIVELLING_FIRESTATE { FIRE_OFF, FIRE_CHARGE };
	enum SHRIVELLING_FIREMODE { FIRE_NARROW, FIRE_WIDE};

	virtual BOOL UseDecrement( void )
	{ 
#if defined( CLIENT_WEAPONS )
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	float				m_shootTime;
	CBeam				*m_pBeam;
	CBeam				*m_pNoise;
	CSprite				*m_pSprite;
	SHRIVELLING_FIRESTATE		m_fireState;
	SHRIVELLING_FIREMODE		m_fireMode;
	float				m_shakeTime;
	BOOL				m_deployed;
};

#endif

