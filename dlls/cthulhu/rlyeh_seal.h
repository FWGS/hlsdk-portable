
#ifndef RLYEH_SEAL_H
#define RLYEH_SEAL_H

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

//////////////////////////////////////////////////////////////////////////////////////////////////

class CFuncRlyehLock : public CBaseEntity
{
public:
	void	Precache( void );
	void	Spawn( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	KeyValue( KeyValueData *pkvd );

	virtual STATE GetState( void ) { return pev->frame?STATE_ON:STATE_OFF; };

	// Bmodels don't go across transitions
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	BOOL	IsLockedByMaster( void );
	void	FireTarget();

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	int			m_sMaster;
	int			m_sTarget;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

#define	RLYEH_SEAL_PRIMARY_VOLUME		450

// The R'lyeh Seal locks Cthulhu into his temple.
// It only attaches itself to a R'lyeh Lock, and only then when the Lock is active (Cthulhu is dead).
// It automatically positions itself above the lock...

class CRlyehSealed : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );

	// no melee attacks
	BOOL CheckMeleeAttack1 ( float flDot, float flDist )	{ return FALSE; };
	BOOL CheckMeleeAttack2 ( float flDot, float flDist )	{ return FALSE; };
	BOOL CheckRangeAttack1 ( float flDot, float flDist )	{ return FALSE; };

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	void EXPORT RlyehThink ( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	//int	Save( CSave &save ); 
	//int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

protected:

};


////////////////////////////////////////////////////////////////////////////////

class CRlyehSeal : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
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
};

#endif



