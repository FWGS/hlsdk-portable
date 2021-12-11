/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
#include "gamerules.h"
#include "sporegrenade.h"

class CSporeAmmo : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT IdleThink ( void );
	void EXPORT AmmoTouch ( CBaseEntity *pOther );
	int  TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );

	virtual int SizeForGrapple() { return GRAPPLE_FIXED; }

	int m_iExplode;
};


typedef enum
{
	SPOREAMMO_IDLE = 0,
	SPOREAMMO_SPAWNUP,
	SPOREAMMO_SNATCHUP,
	SPOREAMMO_SPAWNDOWN,
	SPOREAMMO_SNATCHDOWN,
	SPOREAMMO_IDLE1,
	SPOREAMMO_IDLE2,
} SPOREAMMO;

LINK_ENTITY_TO_CLASS( ammo_spore, CSporeAmmo )

void CSporeAmmo :: Precache( void )
{
	PRECACHE_MODEL("models/spore_ammo.mdl");
	m_iExplode = PRECACHE_MODEL ("sprites/spore_exp_c_01.spr");
	PRECACHE_SOUND("weapons/spore_ammo.wav");
	UTIL_PrecacheOther ( "spore" );
}
//=========================================================
// Spawn
//=========================================================
void CSporeAmmo :: Spawn( void )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/spore_ammo.mdl");
	UTIL_SetSize(pev, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ));
	pev->takedamage = DAMAGE_YES;
	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_NONE;
	pev->framerate		= 1.0f;
	pev->health			= 1.0f;
	pev->animtime		= gpGlobals->time;

	pev->sequence = SPOREAMMO_SPAWNDOWN;
	pev->body = 1;

	pev->origin.z += 16;
	UTIL_SetOrigin( pev, pev->origin );

	pev->angles.x -= 90;// :3

	SetThink (&CSporeAmmo::IdleThink);
	SetTouch (&CSporeAmmo::AmmoTouch);

	pev->nextthink = gpGlobals->time + 4;
}

//=========================================================
// Override all damage
//=========================================================
int CSporeAmmo::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if (pev->body != 0)
	{
		Vector vecSrc = pev->origin + gpGlobals->v_forward * -32;

		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
			WRITE_COORD( vecSrc.x );	// Send to PAS because of the sound
			WRITE_COORD( vecSrc.y );
			WRITE_COORD( vecSrc.z );
			WRITE_SHORT( m_iExplode );
			WRITE_BYTE( 25  ); // scale * 10
			WRITE_BYTE( 12  ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NOSOUND );
		MESSAGE_END();


		//ALERT( at_console, "angles %f %f %f\n", pev->angles.x, pev->angles.y, pev->angles.z );

		Vector angles = pev->angles;
		angles.x -= 90;
		angles.y += 180;

		Vector vecLaunchDir = angles;

		vecLaunchDir.x += RANDOM_FLOAT( -20, 20 );
		vecLaunchDir.y += RANDOM_FLOAT( -20, 20 );
		vecLaunchDir.z += RANDOM_FLOAT( -20, 20 );

		CSpore* pSpore = CSpore::CreateSpore(pev->origin, vecLaunchDir, this, CSpore::GRENADE, false, true);
		UTIL_MakeVectors( vecLaunchDir );
		pSpore->pev->velocity = gpGlobals->v_forward * 800;

		pev->frame = 0;
		pev->animtime		= gpGlobals->time + 0.1;
		pev->sequence		= SPOREAMMO_SNATCHDOWN;
		pev->body			= 0;
		pev->nextthink = gpGlobals->time + 0.66f;
		SetThink (&CSporeAmmo::IdleThink);
		return 1;
	}
	return 0;
}

void CSporeAmmo :: IdleThink ( void )
{
	switch (pev->sequence)
	{
	case SPOREAMMO_SPAWNDOWN:
	{
		pev->sequence = SPOREAMMO_IDLE1;
		pev->animtime = gpGlobals->time;
		pev->frame = 0;
		break;
	}
	case SPOREAMMO_SNATCHDOWN:
	{
		pev->sequence = SPOREAMMO_IDLE;
		pev->animtime = gpGlobals->time;
		pev->frame = 0;
		pev->nextthink = gpGlobals->time + 10.0f;
		break;
	}
	case SPOREAMMO_IDLE:
	{
		pev->body = 1;
		pev->sequence = SPOREAMMO_SPAWNDOWN;
		pev->animtime = gpGlobals->time;
		pev->frame = 0;
		pev->nextthink = gpGlobals->time + 4.0f;
		break;
	}
	default:
		break;
	}
}

void CSporeAmmo :: AmmoTouch ( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() || pev->body == 0 )
		return;

	int bResult = (pOther->GiveAmmo( AMMO_SPORE_GIVE, "spores", SPORE_MAX_CARRY ) != -1);
	if (bResult)
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/spore_ammo.wav", 1, ATTN_NORM);

		pev->frame = 0;
		pev->animtime		= gpGlobals->time;
		pev->sequence = SPOREAMMO_SNATCHDOWN;
		pev->body = 0;
		pev->nextthink = gpGlobals->time + 0.66f;
	}
}
