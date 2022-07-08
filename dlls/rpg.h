/**************************************************************
*																*
*																*
*			= = == Rpg.h == = =									*
*																*
****************************************************************/

//==================
// le fichier rpg.h est necessaire pour pouvoir l inclure 
// dans le rpggrunt.cpp
//
#ifndef RPG_H
#define RPG_H

#include "effects.h"

// viseur

enum
{
	RPG_CROSSHAIR_NORMAL = 0,
	RPG_CROSSHAIR_EMPTY,
	RPG_CROSSHAIR_PROCESS,
	RPG_CROSSHAIR_LOCKED,
};

enum
{
	RPG_TEXT_TOUCHE = 4,
	RPG_TEXT_MANQUE,
};

// menu

#define RPG_MENU_ROCKET_SELECTED		( 1 << 0 )
#define RPG_MENU_ROCKET_EMPTY			( 1 << 1 )
#define RPG_MENU_ELECTRO_SELECTED		( 1 << 2 )
#define RPG_MENU_ELECTRO_EMPTY			( 1 << 3 )
#define RPG_MENU_NUCLEAR_SELECTED		( 1 << 4 )
#define RPG_MENU_NUCLEAR_EMPTY			( 1 << 5 )
#define RPG_MENU_ACTIVE					( 1 << 6 )
#define RPG_CLOSE						( 1 << 7 )
#define RPG_NEUTRE						( 1 << 8 )

// munitions

enum
{
	AMMO_ROCKET = 0,
	AMMO_ELECTRO,
	AMMO_NUCLEAR,
};

#define RPG_MAX_AMMO					5


// submodels

#define RPG_WEAPON_EMPTY				0
#define RPG_WEAPON_ROCKET				1
#define RPG_WEAPON_ELECTRO				2
#define RPG_WEAPON_NUCLEAR				3

// animations

//typedef  //typedef seems to be unneeded now. Roy.
enum rpg_e {
	RPG_IDLE = 0,
	RPG_FIDGET,
	RPG_RELOAD_ROCKET,
	RPG_RELOAD_ELECTRO,
	RPG_RELOAD_NUCLEAR,
	RPG_FIRE,
	RPG_DRAW,
};


//==========================================
//
// = == CRpg
//
//==========================================


class CRpgRocket;

class CRpg : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	void Reload( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	BOOL CanHolster( void );
	void Holster( int skiplocal = 0 );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );

	BOOL ShouldWeaponIdle( void ) { return TRUE; };

	int m_cActiveRockets;// how many missiles in flight from this launcher right now?

	//modif de Julien
	void UpdateEntityTarget ( void );
	void UpdateCrosshair ( int crosshair );
	void UpdateMenu ( void );
	void PlayStateSound ( void );

	int ExtractAmmo( CBasePlayerWeapon *pWeapon );
	int ExtractClipAmmo( CBasePlayerWeapon *pWeapon );
	int GiveAmmo( int iAmount, char *szName, int iMax );

	BOOL AddAmmo ( CBasePlayerWeapon *pWeapon, int iAmmoType, int iNombre );
	virtual void ItemTouch ( CBaseEntity *pOther );

	CBaseEntity *m_pEntityTarget;
	CBaseEntity *m_pEntityLocked;
	CRpgRocket	*pRocket;

	int m_iMenuState;

	float m_flLockTime;
	float m_flReloadTime;

	int m_iAmmoType;
	int m_iAmmoRocket;
	int m_iAmmoElectro;
	int m_iAmmoNuclear;

	BOOL m_bLoaded;
	BOOL m_bRpgUpdate;

	float	m_flLastBip;

};





//==========================================
//
// = == CRpgRocket
//
//==========================================




class CRpgRocket : public CGrenade
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	void EXPORT FollowThink( void );
	void EXPORT IgniteThink( void );
	void EXPORT RocketTouch( CBaseEntity *pOther );
	static CRpgRocket *CreateRpgRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CRpg *pLauncher );

	void EXPORT ElectroThink ( void );

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void Killed( entvars_t *pevAttacker, int iGib );
	
		int m_iTrail;
	float m_flIgniteTime;
	CRpg *m_pLauncher;// pointer back to the launcher that fired me. 

	//modif de Julien
	CBaseEntity *m_pTargetMonster;
	
	int		m_iRocketType;
	float	m_flDiskTime;
	float	m_flLastRadius;

	int		m_iDiskTexture;
	short	m_sNuclearSprite;

};

//==========================================
//
// = == CLaserSpot
//
//==========================================


/*class CLaserSpot : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );

	int	ObjectCaps( void ) { return FCAP_DONT_SAVE; }

public:
	void Suspend( float flSuspendTime );
	void EXPORT Revive( void );
	
	static CLaserSpot *CreateSpot( void );
};
LINK_ENTITY_TO_CLASS( laser_spot, CLaserSpot );*/



#endif		// RPG_H
