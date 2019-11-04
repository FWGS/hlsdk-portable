
#ifndef RIFLE_H
#define RIFLE_H

class CRifle : public CBasePlayerWeapon
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
	void Holster( int skiplocal = 0 );
	void Reload( void );
	void WeaponIdle( void );
	int m_fInReload;
	float m_flNextReload;

	BOOL m_fInZoom;// don't save this. 

	virtual BOOL UseDecrement( void )
	{ 
#if defined( CLIENT_WEAPONS )
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usFireRifle;

};


//////////////////////////////////////////////////////////////////////////////

class CRifleAmmo : public CBasePlayerAmmo
{
	void Spawn( void );
	void Precache( void );
	BOOL AddAmmo( CBaseEntity *pOther );
};





#endif

