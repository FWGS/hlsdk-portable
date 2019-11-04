
#ifndef REVOLVER_H
#define REVOLVER_H

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"


class CRevolver : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void RevolverFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	virtual BOOL UseDecrement( void )
	{ 
#if defined( CLIENT_WEAPONS )
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	int m_iShell;
	

	unsigned short m_usFireRevolver1;
	unsigned short m_usFireRevolver2;
};

//////////////////////////////////////////////////////////////////////////////////////////////

class CRevolverAmmo : public CBasePlayerAmmo
{
	void Spawn( void );
	void Precache( void );
	BOOL AddAmmo( CBaseEntity *pOther ); 
};



#endif


