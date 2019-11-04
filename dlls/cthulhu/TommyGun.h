
#ifndef TOMMYGUN_H
#define TOMMYGUN_H

class CTommyGun : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	float m_flNextAnimTime;
	int m_iShell;

	virtual BOOL UseDecrement( void )
	{ 
#if defined( CLIENT_WEAPONS )
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usTommyGun;
};

class CTommyGunAmmoClip : public CBasePlayerAmmo
{
public:
	void Spawn( void );
	void Precache( void );
	BOOL AddAmmo( CBaseEntity *pOther );
};



#endif

