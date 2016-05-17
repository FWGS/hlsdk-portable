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

#ifndef __AMMO_H__
#define __AMMO_H__

#define MAX_WEAPON_NAME 128


#define WEAPON_FLAGS_SELECTONEMPTY	1

#define WEAPON_IS_ONTARGET 0x40

struct WEAPON
{
	char	szName[MAX_WEAPON_NAME];
	int		iAmmoType;
	int		iAmmo2Type;
	int		iMax1;
	int		iMax2;
	int		iSlot;
	int		iSlotPos;
	int		iFlags;
	int		iId;
	int		iClip;

	int		iCount;		// # of itesm in plist

	SpriteHandle_t hActive;
	wrect_t rcActive;
	SpriteHandle_t hInactive;
	wrect_t rcInactive;
	SpriteHandle_t	hAmmo;
	wrect_t rcAmmo;
	SpriteHandle_t hAmmo2;
	wrect_t rcAmmo2;
	SpriteHandle_t hCrosshair;
	wrect_t rcCrosshair;
	SpriteHandle_t hAutoaim;
	wrect_t rcAutoaim;
	SpriteHandle_t hZoomedCrosshair;
	wrect_t rcZoomedCrosshair;
	SpriteHandle_t hZoomedAutoaim;
	wrect_t rcZoomedAutoaim;
};

typedef int AMMO;


#endif