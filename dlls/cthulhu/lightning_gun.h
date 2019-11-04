
#ifndef LIGHTNING_GUN_H
#define LIGHTNING_GUN_H

class CLightningGun : public CBasePlayerWeapon
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 3; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );
	void ZapPowerUp( void );
	void ZapShoot( void );
	void ZapDone( void );

	int m_fInAttack;

	int m_iBeams;

	void ClearBeams( void );
	void ZapBeam( int side );

	virtual BOOL UseDecrement( void )
	{ 
#if defined( CLIENT_WEAPONS )
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	
	// the beam effect
	CBeam* m_pBeam[LIGHTNING_GUN_MAX_BEAMS];
};

////////////////////////////////////////////////////////////////////////////

class CLightningGunAmmo : public CBasePlayerAmmo
{
	void Spawn( void );
	void Precache( void );
	BOOL AddAmmo( CBaseEntity *pOther );
};

#endif

