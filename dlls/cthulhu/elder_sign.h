
#ifndef ELDERSIGN_H
#define ELDERSIGN_H

/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "effects.h"
#include "gamerules.h"

#include "triggers.h"

#define	ELDER_SIGN_PRIMARY_VOLUME		450


const int MAX_MONSTER = 1000;


// The elder sign is supposed to push all monsters (not humans, like cultists)
// away from it, to prevent them passing...


class CElderSignArea : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );

	// no melee attacks
	BOOL CheckMeleeAttack1 ( float flDot, float flDist )	{ return FALSE; };
	BOOL CheckMeleeAttack2 ( float flDot, float flDist )	{ return FALSE; };
	BOOL CheckRangeAttack1 ( float flDot, float flDist );

	void EXPORT ElderSignThink ( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	void Repel ( CBaseMonster* pEnt );

	//int	Save( CSave &save ); 
	//int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

protected:
	CBaseEntity* mpEntInSphere[MAX_MONSTER];

};


////////////////////////////////////////////////////////////////////////////////

class CElderSign : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 3; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	void SetObjectCollisionBox( void )
	{
		//!!!BUGBUG - fix the model!
		pev->absmin = pev->origin + Vector(-16, -16, -5);
		pev->absmax = pev->origin + Vector(16, 16, 28); 
	}

	void PrimaryAttack( void );
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );
	void WeaponIdle( void );

	virtual BOOL UseDecrement( void )
	{ 
#if defined( CLIENT_WEAPONS )
		return TRUE;
#else
		return FALSE;
#endif
	}
};

#endif



