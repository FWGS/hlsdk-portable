/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// ammohistory.h
//
#pragma once
// this is the max number of items in each bucket
#define MAX_WEAPON_POSITIONS		22

// czeror: the weapons and the accessories (knife, grenades, radio, tools) are
// numbered separately, both from 1. As on the server (ItemInfoArray), the
// accessories are kept after the weapons, at their number + 32. Without it
// the radio (accessory 11) took the place of the glock (weapon 11) and left
// the menu as the player has no glock.
#define MAX_HUD_WEAPONS ( MAX_WEAPONS * 2 )

inline int HUD_WeaponIndex( int iId, int iAccessory )
{
	return iAccessory ? iId + MAX_WEAPONS : iId;
}

class WeaponsResource
{
private:
	// Information about weapons & ammo
	WEAPON		rgWeapons[MAX_HUD_WEAPONS];	// Weapons Array

	// counts of weapons * ammo
	WEAPON*		rgSlots[MAX_WEAPON_SLOTS+1][MAX_WEAPON_POSITIONS+1];	// The slots currently in use by weapons.  The value is a pointer to the weapon;  if it's NULL, no weapon is there
	int			riAmmo[MAX_AMMO_TYPES];							// count of each ammo type

public:
	void Init( void )
	{
		memset( rgWeapons, 0, sizeof rgWeapons );
		Reset();
	}

	void Reset( void )
	{
		iOldWeaponBits = 0;
		iOldAccessoryBits = 0;
		memset( rgSlots, 0, sizeof rgSlots );
		memset( riAmmo, 0, sizeof riAmmo );
	}

///// WEAPON /////
	int			iOldWeaponBits;
	int			iOldAccessoryBits;

	WEAPON *GetWeapon( int iId )
	{
		if ( iId < 0 || iId >= MAX_HUD_WEAPONS )
			return NULL;
		return &rgWeapons[iId];
	}
	void AddWeapon( WEAPON *wp ) 
	{ 
		int idx = HUD_WeaponIndex( wp->iId, wp->iAccessory );
		rgWeapons[ idx ] = *wp;
		LoadWeaponSprites( &rgWeapons[ idx ] );
	}

	void PickupWeapon( WEAPON *wp )
	{
		// czeror: WeapPickup can come before WeaponList. The weapon is empty then (slot 0, position 0) and would take
		// a second place in the menu after its description comes. The weapon bits put it to the right place later.
		if ( !wp->iId )
			return;

		rgSlots[ wp->iSlot ][ wp->iSlotPos ] = wp;
	}

	void PickupWeapon( int idx )
	{
		WEAPON *wp = &rgWeapons[ idx ];
		PickupWeapon( wp );
	}

	void DropWeapon( WEAPON *wp )
	{
		rgSlots[ wp->iSlot ][ wp->iSlotPos ] = NULL;
	}

	void DropAllWeapons( void )
	{
		for ( int i = 0; i < MAX_HUD_WEAPONS; i++ )
		{
			if ( rgWeapons[i].iId )
				DropWeapon( &rgWeapons[i] );
		}
	}

	WEAPON* GetWeaponSlot( int slot, int pos ) { return rgSlots[slot][pos]; }

	void LoadWeaponSprites( WEAPON* wp );
	void LoadAllWeaponSprites( void );
	WEAPON* GetFirstPos( int iSlot );
	void SelectSlot( int iSlot, int fAdvance, int iDirection );
	WEAPON* GetNextActivePos( int iSlot, int iSlotPos );

	int HasAmmo( WEAPON *p );

///// AMMO /////
	void SetAmmo( int iId, int iCount )
	{
		if ( iId < 0 || iId >= MAX_AMMO_TYPES )
			return;
		riAmmo[ iId ] = iCount;
	}

	int CountAmmo( int iId );

	HSPRITE* GetAmmoPicFromWeapon( int iAmmoId, wrect_t& rect );
};

extern WeaponsResource gWR;


#define MAX_HISTORY 12
enum {
	HISTSLOT_EMPTY,
	HISTSLOT_AMMO,
	HISTSLOT_WEAP,
	HISTSLOT_ITEM,
};

class HistoryResource
{
private:
	struct HIST_ITEM {
		int type;
		float DisplayTime;  // the time at which this item should be removed from the history
		int iCount;
		int iId;
	};

	HIST_ITEM rgAmmoHistory[MAX_HISTORY];

public:

	void Init( void )
	{
		Reset();
	}

	void Reset( void )
	{
		memset( rgAmmoHistory, 0, sizeof rgAmmoHistory );
	}

	int iHistoryGap;
	int iCurrentHistorySlot;

	void AddToHistory( int iType, int iId, int iCount = 0 );
	void AddToHistory( int iType, const char *szName, int iCount = 0 );

	void CheckClearHistory( void );
	int DrawAmmoHistory( float flTime );
};

extern HistoryResource gHR;



